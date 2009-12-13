#include "filter/filter.h"

class VideoInfo
{
public:

	int width;
	int height;

	int rotation;
	int rotation_userset;
	int screengap;
	int layout;
	int layout_old;
	int	swap;

	int currentfilter;

	u8* srcBuffer;
	CACHE_ALIGN u32 buffer[4*256*192*2];
	CACHE_ALIGN u32 filteredbuffer[4*256*192*2];

	enum {
		NONE,
		HQ2X,
		_2XSAI,
		SUPER2XSAI,
		SUPEREAGLE,
		SCANLINE,
		BILINEAR,
		NEAREST2X,
		HQ2XS,
		LQ2X,
		LQ2XS,
        EPX,
        NEARESTPLUS1POINT5,
        NEAREST1POINT5,
        EPXPLUS,
        EPX1POINT5,
        EPXPLUS1POINT5,

		NUM_FILTERS,
	};


	void reset() {
		width = 256;
		height = 384;
	}

	void setfilter(int filter) {

		if(filter < 0 || filter >= NUM_FILTERS)
			filter = NONE;

		currentfilter = filter;

		switch(filter) {

			case NONE:
				width = 256;
				height = 384;
				break;
			case EPX1POINT5:
			case EPXPLUS1POINT5:
			case NEAREST1POINT5:
			case NEARESTPLUS1POINT5:
				width = 256*3/2;
				height = 384*3/2;
				break;
			default:
				width = 256*2;
				height = 384*2;
				break;
		}
	}

	SSurface src;
	SSurface dst;

	u16* finalBuffer() const
	{
		if(currentfilter == NONE)
			return (u16*)buffer;
		else return (u16*)filteredbuffer;
	}

	void filter() {

		src.Height = 384;
		src.Width = 256;
		src.Pitch = 512;
		src.Surface = (u8*)buffer;

		dst.Height = height;
		dst.Width = width;
		dst.Pitch = width*2;
		dst.Surface = (u8*)filteredbuffer;

		switch(currentfilter)
		{
			case NONE:
				break;
			case LQ2X:
				RenderLQ2X(src, dst);
				break;
			case LQ2XS:
				RenderLQ2XS(src, dst);
				break;
			case HQ2X:
				RenderHQ2X(src, dst);
				break;
			case HQ2XS:
				RenderHQ2XS(src, dst);
				break;
			case _2XSAI:
				Render2xSaI (src, dst);
				break;
			case SUPER2XSAI:
				RenderSuper2xSaI (src, dst);
				break;
			case SUPEREAGLE:
				RenderSuperEagle (src, dst);
				break;
			case SCANLINE:
				RenderScanline(src, dst);
				break;
			case BILINEAR:
				RenderBilinear(src, dst);
				break;
			case NEAREST2X:
				RenderNearest2X(src,dst);
				break;
			case EPX:
				RenderEPX(src,dst);
				break;
			case EPXPLUS:
				RenderEPXPlus(src,dst);
				break;
			case EPX1POINT5:
				RenderEPX_1Point5x(src,dst);
				break;
			case EPXPLUS1POINT5:
				RenderEPXPlus_1Point5x(src,dst);
				break;
			case NEAREST1POINT5:
				RenderNearest_1Point5x(src,dst);
				break;
			case NEARESTPLUS1POINT5:
				RenderNearestPlus_1Point5x(src,dst);
				break;
		}
	}

	int size() {
		return width*height;
	}

	int dividebyratio(int x) {
		return x * 256 / width;
	}

	int rotatedwidth() {
		switch(rotation) {
			case 0:
				return width;
			case 90:
				return height;
			case 180:
				return width;
			case 270:
				return height;
			default:
				return 0;
		}
	}

	int rotatedheight() {
		switch(rotation) {
			case 0:
				return height;
			case 90:
				return width;
			case 180:
				return height;
			case 270:
				return width;
			default:
				return 0;
		}
	}

	int rotatedwidthgap() {
		switch(rotation) {
			case 0:
				return width;
			case 90:
				return height + ((layout == 0) ? scaledscreengap() : 0);
			case 180:
				return width;
			case 270:
				return height + ((layout == 0) ? scaledscreengap() : 0);
			default:
				return 0;
		}
	}

	int rotatedheightgap() {
		switch(rotation) {
			case 0:
				return height + ((layout == 0) ? scaledscreengap() : 0);
			case 90:
				return width;
			case 180:
				return height + ((layout == 0) ? scaledscreengap() : 0);
			case 270:
				return width;
			default:
				return 0;
		}
	}

	int scaledscreengap() {
		return screengap * height / 384;
	}
};
