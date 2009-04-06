/* movie.cpp
 *
 * Copyright (C) 2006-2008 Zeromus
 *
 * This file is part of DeSmuME
 *
 * DeSmuME is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DeSmuME is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DeSmuME; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <assert.h>
#include <limits.h>
#include <fstream>

#include "utils/guid.h"
#include "utils/xstring.h"
#include "movie.h"
#include "NDSSystem.h"
#include "debug.h"

using namespace std;

#define FCEU_PrintError LOG

#define MOVIE_VERSION 1

//----movie engine main state

EMOVIEMODE movieMode = MOVIEMODE_INACTIVE;

//this should not be set unless we are in MOVIEMODE_RECORD!
fstream* osRecordingMovie = 0;

int currFrameCounter;
uint32 cur_input_display = 0;
int pauseframe = -1;
bool movie_readonly = true;

char curMovieFilename[512] = {0};
MovieData currMovieData;
int currRerecordCount;

//--------------


void MovieData::clearRecordRange(int start, int len)
{
	for(int i=0;i<len;i++)
		records[i+start].clear();
}

void MovieData::insertEmpty(int at, int frames)
{
	if(at == -1) 
	{
		int currcount = records.size();
		records.resize(records.size()+frames);
		clearRecordRange(currcount,frames);
	}
	else
	{
		records.insert(records.begin()+at,frames,MovieRecord());
		clearRecordRange(at,frames);
	}
}



void MovieRecord::clear()
{ 
	pad = 0;
	commands = 0;
	touch.padding = 0;
}


const char MovieRecord::mnemonics[13] = {'R','L','D','U','T','S','B','A','Y','X','W','E','G'};
void MovieRecord::dumpPad(std::ostream* os, u16 pad)
{
	//these are mnemonics for each joystick bit.
	//since we usually use the regular joypad, these will be more helpful.
	//but any character other than ' ' or '.' should count as a set bit
	//maybe other input types will need to be encoded another way..
	for(int bit=0;bit<13;bit++)
	{
		int bitmask = (1<<(12-bit));
		char mnemonic = mnemonics[bit];
		//if the bit is set write the mnemonic
		if(pad & bitmask)
			os->put(mnemonic);
		else //otherwise write an unset bit
			os->put('.');
	}
}


void MovieRecord::parsePad(std::istream* is, u16& pad)
{
	char buf[13];
	is->read(buf,13);
	pad = 0;
	for(int i=0;i<13;i++)
	{
		pad <<= 1;
		pad |= ((buf[i]=='.'||buf[i]==' ')?0:1);
	}
}


void MovieRecord::parse(MovieData* md, std::istream* is)
{
	//by the time we get in here, the initial pipe has already been extracted

	//extract the commands
	commands = u32DecFromIstream(is);
	
	is->get(); //eat the pipe

	parsePad(is, pad);
	touch.x = u32DecFromIstream(is);
	touch.y = u32DecFromIstream(is);
	touch.touch = u32DecFromIstream(is);
		
	is->get(); //eat the pipe

	//should be left at a newline
}


void MovieRecord::dump(MovieData* md, std::ostream* os, int index)
{
	//dump the misc commands
	//*os << '|' << setw(1) << (int)commands;
	os->put('|');
	putdec<uint8,1,true>(os,commands);

	os->put('|');
	dumpPad(os, pad);
	putdec<u8,3,true>(os,touch.x); os->put(' ');
	putdec<u8,3,true>(os,touch.y); os->put(' ');
	putdec<u8,1,true>(os,touch.touch);
	os->put('|');
	
	//each frame is on a new line
	os->put('\n');
}

MovieData::MovieData()
	: version(MOVIE_VERSION)
	, emuVersion(DESMUME_VERSION_NUMERIC)
	, rerecordCount(1)
	, binaryFlag(false)
	//, greenZoneCount(0)
{
	memset(&romChecksum,0,sizeof(MD5DATA));
}

void MovieData::truncateAt(int frame)
{
	records.resize(frame);
}


void MovieData::installValue(std::string& key, std::string& val)
{
	//todo - use another config system, or drive this from a little data structure. because this is gross
	if(key == "version")
		installInt(val,version);
	else if(key == "emuVersion")
		installInt(val,emuVersion);
	else if(key == "rerecordCount")
		installInt(val,rerecordCount);
	else if(key == "romFilename")
		romFilename = val;
	else if(key == "romChecksum")
		StringToBytes(val,&romChecksum,MD5DATA::size);
	else if(key == "guid")
		guid = Desmume_Guid::fromString(val);
	else if(key == "comment")
		comments.push_back(mbstowcs(val));
	else if(key == "savestate")
	{
		int len = Base64StringToBytesLength(val);
		if(len == -1) len = HexStringToBytesLength(val); // wasn't base64, try hex
		if(len >= 1)
		{
			savestate.resize(len);
			StringToBytes(val,&savestate[0],len); // decodes either base64 or hex
		}
	}
}


int MovieData::dump(std::ostream *os, bool binary)
{
	int start = os->tellp();
	*os << "version " << version << endl;
	*os << "emuVersion " << emuVersion << endl;
	*os << "rerecordCount " << rerecordCount << endl;
	*os << "romFilename " << romFilename << endl;
	*os << "romChecksum " << BytesToString(romChecksum.data,MD5DATA::size) << endl;
	*os << "guid " << guid.toString() << endl;

	for(uint32 i=0;i<comments.size();i++)
		*os << "comment " << wcstombs(comments[i]) << endl;
	
	if(binary)
		*os << "binary 1" << endl;
		
	if(savestate.size() != 0)
		*os << "savestate " << BytesToString(&savestate[0],savestate.size()) << endl;
	if(binary)
	{
		////put one | to start the binary dump
		//os->put('|');
		//for(int i=0;i<(int)records.size();i++)
		//	records[i].dumpBinary(this,os,i);
	}
	else
		for(int i=0;i<(int)records.size();i++)
			records[i].dump(this,os,i);

	int end = os->tellp();
	return end-start;
}

//yuck... another custom text parser.
static bool LoadFM2(MovieData& movieData, std::istream* fp, int size, bool stopAfterHeader)
{
	//TODO - start with something different. like 'desmume movie version 1"
	std::ios::pos_type curr = fp->tellg();

	//movie must start with "version 1"
	char buf[9];
	curr = fp->tellg();
	fp->read(buf,9);
	fp->seekg(curr);
	if(fp->fail()) return false;
	if(memcmp(buf,"version 1",9)) 
		return false;

	std::string key,value;
	enum {
		NEWLINE, KEY, SEPARATOR, VALUE, RECORD, COMMENT
	} state = NEWLINE;
	bool bail = false;
	for(;;)
	{
		bool iswhitespace, isrecchar, isnewline;
		int c;
		if(size--<=0) goto bail;
		c = fp->get();
		if(c == -1)
			goto bail;
		iswhitespace = (c==' '||c=='\t');
		isrecchar = (c=='|');
		isnewline = (c==10||c==13);
		if(isrecchar && movieData.binaryFlag && !stopAfterHeader)
		{
			//LoadFM2_binarychunk(movieData, fp, size);
			return false;
		}
		switch(state)
		{
		case NEWLINE:
			if(isnewline) goto done;
			if(iswhitespace) goto done;
			if(isrecchar) 
				goto dorecord;
			//must be a key
			key = "";
			value = "";
			goto dokey;
			break;
		case RECORD:
			{
				dorecord:
				if (stopAfterHeader) return true;
				int currcount = movieData.records.size();
				movieData.records.resize(currcount+1);
				int preparse = fp->tellg();
				movieData.records[currcount].parse(&movieData, fp);
				int postparse = fp->tellg();
				size -= (postparse-preparse);
				state = NEWLINE;
				break;
			}

		case KEY:
			dokey: //dookie
			state = KEY;
			if(iswhitespace) goto doseparator;
			if(isnewline) goto commit;
			key += c;
			break;
		case SEPARATOR:
			doseparator:
			state = SEPARATOR;
			if(isnewline) goto commit;
			if(!iswhitespace) goto dovalue;
			break;
		case VALUE:
			dovalue:
			state = VALUE;
			if(isnewline) goto commit;
			value += c;
			break;
		case COMMENT:
		default:
			break;
		}
		goto done;

		bail:
		bail = true;
		if(state == VALUE) goto commit;
		goto done;
		commit:
		movieData.installValue(key,value);
		state = NEWLINE;
		done: ;
		if(bail) break;
	}

	return true;
}


static void closeRecordingMovie()
{
	if(osRecordingMovie)
	{
		delete osRecordingMovie;
		osRecordingMovie = 0;
	}
}

/// Stop movie playback.
static void StopPlayback()
{
	//FCEU_DispMessageOnMovie("Movie playback stopped.");
	movieMode = MOVIEMODE_INACTIVE;
}


/// Stop movie recording
static void StopRecording()
{
	//FCEU_DispMessage("Movie recording stopped.");

	movieMode = MOVIEMODE_INACTIVE;
	
	closeRecordingMovie();
}



static void FCEUI_StopMovie()
{
	//if(suppressMovieStop)
	//	return;
	
	if(movieMode == MOVIEMODE_PLAY)
		StopPlayback();
	else if(movieMode == MOVIEMODE_RECORD)
		StopRecording();

	curMovieFilename[0] = 0;
}


//begin playing an existing movie
static void FCEUI_LoadMovie(const char *fname, bool _read_only, bool tasedit, int _pauseframe)
{
	//if(!tasedit && !FCEU_IsValidUI(FCEUI_PLAYMOVIE))
	//	return;

	assert(fname);

	//mbg 6/10/08 - we used to call StopMovie here, but that cleared curMovieFilename and gave us crashes...
	if(movieMode == MOVIEMODE_PLAY)
		StopPlayback();
	else if(movieMode == MOVIEMODE_RECORD)
		StopRecording();
	//--------------

	currMovieData = MovieData();
	
	strcpy(curMovieFilename, fname);
	//FCEUFILE *fp = FCEU_fopen(fname,0,"rb",0);
	//if (!fp) return;
	//if(fp->isArchive() && !_read_only) {
	//	FCEU_PrintError("Cannot open a movie in read+write from an archive.");
	//	return;
	//}

	//LoadFM2(currMovieData, fp->stream, INT_MAX, false);

	
	fstream fs (fname);
	LoadFM2(currMovieData, &fs, INT_MAX, false);
	fs.close();

	//TODO
	//fully reload the game to reinitialize everything before playing any movie
	//poweron(true);

	////WE NEED TO LOAD A SAVESTATE
	//if(currMovieData.savestate.size() != 0)
	//{
	//	bool success = MovieData::loadSavestateFrom(&currMovieData.savestate);
	//	if(!success) return;
	//}

	currFrameCounter = 0;
	pauseframe = _pauseframe;
	movie_readonly = _read_only;
	movieMode = MOVIEMODE_PLAY;
	currRerecordCount = currMovieData.rerecordCount;

	//if(movie_readonly)
	//	FCEU_DispMessage("Replay started Read-Only.");
	//else
	//	FCEU_DispMessage("Replay started Read+Write.");
}

static void openRecordingMovie(const char* fname)
{
	//osRecordingMovie = FCEUD_UTF8_fstream(fname, "wb");
	osRecordingMovie = new fstream(fname,std::ios_base::out);
	/*if(!osRecordingMovie)
		FCEU_PrintError("Error opening movie output file: %s",fname);*/
	strcpy(curMovieFilename, fname);
}


//begin recording a new movie
//TODO - BUG - the record-from-another-savestate doesnt work.
static void FCEUI_SaveMovie(const char *fname, std::wstring author)
{
	//if(!FCEU_IsValidUI(FCEUI_RECORDMOVIE))
	//	return;

	assert(fname);

	FCEUI_StopMovie();

	openRecordingMovie(fname);

	currFrameCounter = 0;
	//LagCounterReset();

	currMovieData = MovieData();
	currMovieData.guid.newGuid();

	if(author != L"") currMovieData.comments.push_back(L"author " + author);
	//currMovieData.romChecksum = GameInfo->MD5;
	//currMovieData.romFilename = FileBase;

	//todo ?
	//poweron(true);
	//else
	//	MovieData::dumpSavestateTo(&currMovieData.savestate,Z_BEST_COMPRESSION);

	//we are going to go ahead and dump the header. from now on we will only be appending frames
	currMovieData.dump(osRecordingMovie, false);

	movieMode = MOVIEMODE_RECORD;
	movie_readonly = false;
	currRerecordCount = 0;
	
	//FCEU_DispMessage("Movie recording started.");
}


//the main interaction point between the emulator and the movie system.
//either dumps the current joystick state or loads one state from the movie
static void FCEUMOV_AddInputState()
{
	//todo - for tasedit, either dump or load depending on whether input recording is enabled
	//or something like that
	//(input recording is just like standard read+write movie recording with input taken from gamepad)
	//otherwise, it will come from the tasedit data.

	if(movieMode == MOVIEMODE_PLAY)
	{
		//stop when we run out of frames
		if(currFrameCounter == (int)currMovieData.records.size())
		{
			StopPlayback();
		}
		else
		{
			MovieRecord* mr = &currMovieData.records[currFrameCounter];
			
			//reset if necessary
			if(mr->command_reset())
			{}
				//ResetNES();

			NDS_setPadFromMovie(mr->pad);
		}

		//if we are on the last frame, then pause the emulator if the player requested it
		if(currFrameCounter == (int)currMovieData.records.size()-1)
		{
			/*if(FCEUD_PauseAfterPlayback())
			{
				FCEUI_ToggleEmulationPause();
			}*/
		}

		//pause the movie at a specified frame 
		//if(FCEUMOV_ShouldPause() && FCEUI_EmulationPaused()==0)
		//{
		//	FCEUI_ToggleEmulationPause();
		//	FCEU_DispMessage("Paused at specified movie frame");
		//}
		
	}
	else if(movieMode == MOVIEMODE_RECORD)
	{
		MovieRecord mr;

		mr.commands = 0;
		mr.pad = nds.pad;
		if(nds.isTouch) {
			mr.touch.x = nds.touchX;
			mr.touch.y = nds.touchY;
			mr.touch.touch = 1;
		} else {
			mr.touch.x = 0;
			mr.touch.y = 0;
			mr.touch.touch = 0;
		}

		mr.dump(&currMovieData, osRecordingMovie,currMovieData.records.size());
		currMovieData.records.push_back(mr);
	}

	currFrameCounter++;

	/*extern uint8 joy[4];
	memcpy(&cur_input_display,joy,4);*/
}


//TODO 
static void FCEUMOV_AddCommand(int cmd)
{
	// do nothing if not recording a movie
	if(movieMode != MOVIEMODE_RECORD)
		return;
	
	//printf("%d\n",cmd);

	//MBG TODO BIG TODO TODO TODO
	//DoEncode((cmd>>3)&0x3,cmd&0x7,1);
}


static int FCEUMOV_WriteState(std::ostream* os)
{
	//we are supposed to dump the movie data into the savestate
	if(movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY)
		return currMovieData.dump(os, true);
	else return 0;
}


//TODO EVERYTHING BELOW

static bool load_successful;

static bool FCEUMOV_ReadState(std::istream* is, uint32 size)
{
	load_successful = false;

	//a little rule: cant load states in read+write mode with a movie from an archive.
	//so we are going to switch it to readonly mode in that case
	if(!movie_readonly 
		//*&& FCEU_isFileInArchive(curMovieFilename)*/
		) {
		FCEU_PrintError("Cannot loadstate in Read+Write with movie from archive. Movie is now Read-Only.");
		movie_readonly = true;
	}

	MovieData tempMovieData = MovieData();
	std::ios::pos_type curr = is->tellg();
	if(!LoadFM2(tempMovieData, is, size, false)) {
		
		/*is->seekg((uint32)curr+size);
		extern bool FCEU_state_loading_old_format;
		if(FCEU_state_loading_old_format) {
			if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD) {
				FCEUI_StopMovie();			
				FCEU_PrintError("You have tried to use an old savestate while playing a movie. This is unsupported (since the old savestate has old-format movie data in it which can't be converted on the fly)");
			}
		}*/
		return false;
	}

	//complex TAS logic for when a savestate is loaded:
	//----------------
	//if we are playing or recording and toggled read-only:
	//  then, the movie we are playing must match the guid of the one stored in the savestate or else error.
	//  the savestate is assumed to be in the same timeline as the current movie.
	//  if the current movie is not long enough to get to the savestate's frame#, then it is an error. 
	//  the movie contained in the savestate will be discarded.
	//  the emulator will be put into play mode.
	//if we are playing or recording and toggled read+write
	//  then, the movie we are playing must match the guid of the one stored in the savestate or else error.
	//  the movie contained in the savestate will be loaded into memory
	//  the frames in the movie after the savestate frame will be discarded
	//  the in-memory movie will have its rerecord count incremented
	//  the in-memory movie will be dumped to disk as fcm.
	//  the emulator will be put into record mode.
	//if we are doing neither:
	//  then, we must discard this movie and just load the savestate


	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD)
	{
		//handle moviefile mismatch
		if(tempMovieData.guid != currMovieData.guid)
		{
			//mbg 8/18/08 - this code  can be used to turn the error message into an OK/CANCEL
			#ifdef WIN32
				//std::string msg = "There is a mismatch between savestate's movie and current movie.\ncurrent: " + currMovieData.guid.toString() + "\nsavestate: " + tempMovieData.guid.toString() + "\n\nThis means that you have loaded a savestate belonging to a different movie than the one you are playing now.\n\nContinue loading this savestate anyway?";
				//extern HWND pwindow;
				//int result = MessageBox(pwindow,msg.c_str(),"Error loading savestate",MB_OKCANCEL);
				//if(result == IDCANCEL)
				//	return false;
			#else
				FCEU_PrintError("Mismatch between savestate's movie and current movie.\ncurrent: %s\nsavestate: %s\n",currMovieData.guid.toString().c_str(),tempMovieData.guid.toString().c_str());
				return false;
			#endif
		}

		closeRecordingMovie();

		if(movie_readonly)
		{
			//if the frame counter is longer than our current movie, then error
			if(currFrameCounter > (int)currMovieData.records.size())
			{
				FCEU_PrintError("Savestate is from a frame (%d) after the final frame in the movie (%d). This is not permitted.", currFrameCounter, currMovieData.records.size()-1);
				return false;
			}
			movieMode = MOVIEMODE_PLAY;
		}
		else
		{
			//truncate before we copy, just to save some time
			tempMovieData.truncateAt(currFrameCounter);
			currMovieData = tempMovieData;
			
			#ifdef _S9XLUA_H
			if(!FCEU_LuaRerecordCountSkip())
				currRerecordCount++;
			#endif
			
			currMovieData.rerecordCount = currRerecordCount;

			openRecordingMovie(curMovieFilename);
			currMovieData.dump(osRecordingMovie, false);
			movieMode = MOVIEMODE_RECORD;
		}
	}
	
	load_successful = true;

	//// Maximus: Show the last input combination entered from the
	//// movie within the state
	//if(current!=0) // <- mz: only if playing or recording a movie
	//	memcpy(&cur_input_display, joop, 4);

	return true;
}

static void FCEUMOV_PreLoad(void)
{
	load_successful=0;
}

static bool FCEUMOV_PostLoad(void)
{
	if(movieMode == MOVIEMODE_INACTIVE)
		return true;
	else
		return load_successful;
}


bool FCEUI_MovieGetInfo(std::istream* fp, MOVIE_INFO& info, bool skipFrameCount)
{
	//MovieData md;
	//if(!LoadFM2(md, fp, INT_MAX, skipFrameCount))
	//	return false;
	//
	//info.movie_version = md.version;
	//info.poweron = md.savestate.size()==0;
	//info.pal = md.palFlag;
	//info.nosynchack = true;
	//info.num_frames = md.records.size();
	//info.md5_of_rom_used = md.romChecksum;
	//info.emu_version_used = md.emuVersion;
	//info.name_of_rom_used = md.romFilename;
	//info.rerecord_count = md.rerecordCount;
	//info.comments = md.comments;

	return true;
}
