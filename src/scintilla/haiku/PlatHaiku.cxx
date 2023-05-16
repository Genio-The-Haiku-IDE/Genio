// Scintilla source code edit control
// PlatHaiku.cxx - implementation of platform facilities on Haiku
// Copyright 2011, 2022 by Andrea Anzani <andrea.anzani@gmail.com>
// Copyright 2014-2021 by Kacper Kasper <kacperkasper@gmail.com>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <sys/time.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Font.h>
#include <GradientLinear.h>
#include <GroupLayout.h>
#include <ListItem.h>
#include <ListView.h>
#include <PopUpMenu.h>
#include <Picture.h>
#include <Region.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Shape.h>
#include <String.h>
#include <Window.h>
#include <View.h>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Scintilla.h"
#include "XPM.h"
#include "UniConversion.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

namespace {

const Supports SupportsHaiku[] = {
	Supports::LineDrawsFinal,
	Supports::FractionalStrokeWidth,
	Supports::TranslucentStroke,
	Supports::PixelModification,
};

class BitmapLock
{
public:
	BitmapLock(BBitmap *b) {
		bit = b;
		if (bit) b->Lock();
	}
	~BitmapLock() {
		if(bit) {
			BView* drawer = bit->ChildAt(0);
			if (drawer)
				drawer->Sync();
			bit->Unlock();
		}
	}

private:
	BBitmap* bit;
};

BRect toBRect(PRectangle r) {
	// Haiku renders rectangles with left == right or top == bottom
	// as they were 1 pixel wide
	//if(r.left == r.right || r.top == r.bottom) debugger("INVALID RECT");
	return BRect(r.left, r.top, r.right - 1, r.bottom - 1);
}

PRectangle toPRect(BRect r) {
	return PRectangle(r.left, r.top, r.right + 1, r.bottom + 1);
}

BPoint toBPoint(Point p) {
	return BPoint(p.x, p.y);
}

struct FontHaiku : public Font {
	BFont *bfont;
	FontHaiku() noexcept : bfont(nullptr) {
	}
	FontHaiku(const FontParameters &fp) {
		bfont = new BFont();
		bfont->SetFamilyAndStyle(fp.faceName, nullptr);
		uint16 face = 0;
		if (fp.weight > Scintilla::FontWeight::Normal)
			face |= B_BOLD_FACE;
		if (fp.italic == true)
			face |= B_ITALIC_FACE;
		bfont->SetFace(face);
		bfont->SetSize(fp.size);
	}
	FontHaiku(const FontHaiku &) = delete;
	FontHaiku(FontHaiku &&) = delete;
	FontHaiku &operator=(const FontHaiku &) = delete;
	FontHaiku &operator=(FontHaiku &&) = delete;
	~FontHaiku() noexcept override {
		delete bfont;
	}
};

const FontHaiku *HFont(const Font *f) noexcept {
	return dynamic_cast<const FontHaiku *>(f);
}

rgb_color bcolor(ColourRGBA col) {
	rgb_color hcol;
	hcol.red = col.GetRed();
	hcol.green = col.GetGreen();
	hcol.blue = col.GetBlue();
	hcol.alpha = 255;//!
	return hcol;
}

BView* bview(WindowID id) {
	return dynamic_cast<BView*>(static_cast<BHandler*>(id));
}

BWindow* bwindow(WindowID id) {
	return dynamic_cast<BWindow*>(static_cast<BHandler*>(id));
}

int UTF8CharLength(const char* text) {
	auto ch = reinterpret_cast<const unsigned char*>(text);
	return UTF8Classify(ch, 4);
}

int MultibyteToUTF8Length(std::string_view text) {
	int length = 0;
	for(size_t i = 0; i < text.length(); i += UTF8CharLength(text.data() + i)) {
		length++;
	}
	return length;
}

}

namespace Scintilla {

std::shared_ptr<Font> Font::Allocate(const FontParameters &fp) {
	return std::make_shared<FontHaiku>(fp);
}

class SurfaceImpl : public Surface {
private:
	BView* 	  ownerView;
	BBitmap*  bitmap;

public:
	SurfaceImpl() noexcept;
	SurfaceImpl(int width, int height) noexcept;
	// Deleted so SurfaceImpl objects can not be copied.
	SurfaceImpl(const SurfaceImpl&) = delete;
	SurfaceImpl(SurfaceImpl&&) = delete;
	SurfaceImpl&operator=(const SurfaceImpl&) = delete;
	SurfaceImpl&operator=(SurfaceImpl&&) = delete;
	~SurfaceImpl() override;

	BBitmap* GetBitmap() { return bitmap; }

	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	std::unique_ptr<Surface> AllocatePixMap(int width, int height) override;

	void SetMode(SurfaceMode mode_) override;

	void Release() noexcept override;
	int SupportsFeature(Supports feature) noexcept override;
	bool Initialised() override;
	int LogPixelsY() override;
	int PixelDivisions() override;
	int DeviceHeightFont(int points) override;
	void LineDraw(Point start, Point end, Stroke stroke) override;
	void PolyLine(const Point *pts, size_t npts, Stroke stroke) override;
	void Polygon(const Point *pts, size_t npts, FillStroke fillStroke) override;
	void RectangleDraw(PRectangle rc, FillStroke fillStroke) override;
	void RectangleFrame(PRectangle rc, Stroke stroke) override;
	void FillRectangle(PRectangle rc, Fill fill) override;
	void FillRectangleAligned(PRectangle rc, Fill fill) override;
	void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
	void RoundedRectangle(PRectangle rc, FillStroke fillStroke) override;
	void AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) override;
	void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops,
		GradientOptions options) override;
	void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) override;
	void Ellipse(PRectangle rc, FillStroke fillStroke) override;
	void Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) override;
	void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

	std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

	void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font_, std::string_view text) override;

	void DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore) override;
	void MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthTextUTF8(const Font *font_, std::string_view text) override;

	XYPOSITION Ascent(const Font *font_) override;
	XYPOSITION Descent(const Font *font_) override;
	XYPOSITION InternalLeading(const Font *font_) override;
	XYPOSITION Height(const Font *font_) override;
	XYPOSITION AverageCharWidth(const Font *font_) override;

	void SetClip(PRectangle rc) override;
	void PopClip() override;
	void FlushCachedState() override;
	void FlushDrawing() override;
};

SurfaceImpl::SurfaceImpl() noexcept {
	ownerView = nullptr;
	bitmap = nullptr;
}

SurfaceImpl::SurfaceImpl(int width, int height) noexcept {
	if(width > 0 && height > 0) {
		ownerView = new BView(BRect(0, 0, width - 1, height - 1), "pixMap", B_FOLLOW_ALL, B_WILL_DRAW);
		bitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32, true, false);
		bitmap->AddChild(ownerView);
	}
}

SurfaceImpl::~SurfaceImpl() {
	Release();
}

void SurfaceImpl::Init(WindowID wid) {
	Release();

	ownerView = static_cast<BView*>( wid );
}

void SurfaceImpl::Init(SurfaceID sid, WindowID wid) {
	Release();

	ownerView = static_cast<BView*>( sid );
}

std::unique_ptr<Surface> SurfaceImpl::AllocatePixMap(int width, int height) {
	return std::make_unique<SurfaceImpl>(width, height);
}

void SurfaceImpl::SetMode(SurfaceMode) {
	// TODO
}

void SurfaceImpl::Release() noexcept {
	if (bitmap) {
		bitmap->Lock();
		bitmap->RemoveChild(ownerView);
		delete bitmap;
		delete ownerView;
		bitmap = nullptr;
		ownerView = nullptr;
	}
}

int SurfaceImpl::SupportsFeature(Supports feature) noexcept {
	for (const Supports f : SupportsHaiku) {
		if (f == feature)
			return 1;
	}
	return 0;
}

bool SurfaceImpl::Initialised() {
	return ownerView != nullptr;
}

int SurfaceImpl::LogPixelsY() {
	return 72;
}

int SurfaceImpl::PixelDivisions() {
	return 1;
}

int SurfaceImpl::DeviceHeightFont(int points) {
	int logPix = LogPixelsY();
	return (points * logPix + logPix / 2) / 72;
}

void SurfaceImpl::LineDraw(Point start, Point end, Stroke stroke) {
	BitmapLock l(bitmap);
	ownerView->SetPenSize(stroke.width);
	ownerView->SetHighColor(bcolor(stroke.colour));
	ownerView->StrokeLine(toBPoint(start), toBPoint(end));
}

void SurfaceImpl::PolyLine(const Point *pts, size_t npts, Stroke stroke) {
	std::vector<BPoint> points(npts);
	for (size_t i = 0; i < npts; i++)
	{
		points[i].x = pts[i].x;
		points[i].y = pts[i].y;
	}
	BitmapLock l(bitmap);
	ownerView->SetPenSize(stroke.width);
	ownerView->SetHighColor(bcolor(stroke.colour));
	ownerView->StrokePolygon(points.data(), points.size(), false);
}

void SurfaceImpl::Polygon(const Point *pts, size_t npts, FillStroke fillStroke) {
	std::vector<BPoint> points(npts);
	for (size_t i = 0; i < npts; i++)
	{
		points[i].x = pts[i].x;
		points[i].y = pts[i].y;
	}
	BitmapLock l(bitmap);
	ownerView->SetHighColor(bcolor(fillStroke.fill.colour));
	ownerView->FillPolygon(points.data(), points.size());
	ownerView->SetPenSize(fillStroke.stroke.width);
	ownerView->SetHighColor(bcolor(fillStroke.stroke.colour));
	ownerView->StrokePolygon(points.data(), points.size());
}

void SurfaceImpl::RectangleDraw(PRectangle rc, FillStroke fillStroke) {
	BRect rect(toBRect(rc));

	BitmapLock l(bitmap);
	ownerView->SetHighColor(bcolor(fillStroke.fill.colour));
	ownerView->FillRect(rect);
	ownerView->SetPenSize(fillStroke.stroke.width);
	ownerView->SetHighColor(bcolor(fillStroke.stroke.colour));
	ownerView->StrokeRect(rect);
}

void SurfaceImpl::RectangleFrame(PRectangle rc, Stroke stroke) {
	BRect rect(toBRect(rc));

	BitmapLock l(bitmap);
	ownerView->SetPenSize(stroke.width);
	ownerView->SetHighColor(bcolor(stroke.colour));
	ownerView->StrokeRect(rect);
}

void SurfaceImpl::FillRectangle(PRectangle rc, Fill fill) {
	BitmapLock l(bitmap);
	ownerView->SetHighColor(bcolor(fill.colour));
	ownerView->FillRect(toBRect(rc));
}

void SurfaceImpl::FillRectangleAligned(PRectangle rc, Fill fill) {
	FillRectangle(PixelAlign(rc, 1), fill);
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceImpl& pattern = static_cast<SurfaceImpl &>(surfacePattern);

	BBitmap* image = pattern.GetBitmap();
	// If we could not get an image reference, fill the rectangle black
	if (image == nullptr) {
		FillRectangle(rc, Fill(ColourRGBA(0)));
		return;
	}

	BRect destRect = toBRect(rc);
	BitmapLock l(bitmap);
	ownerView->DrawTiledBitmapAsync(image, destRect);
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, FillStroke fillStroke) {
	BRect rect(toBRect(rc));
	const int radius = 3;

	BitmapLock l(bitmap);
	ownerView->SetHighColor(bcolor(fillStroke.fill.colour));
	ownerView->FillRoundRect(rect, radius, radius);
	ownerView->SetPenSize(fillStroke.stroke.width);
	ownerView->SetHighColor(bcolor(fillStroke.stroke.colour));
	ownerView->StrokeRoundRect(rect, radius, radius);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) {
	BRect rect(toBRect(rc));
	rgb_color back, fore;
	back = bcolor(fillStroke.fill.colour);
	back.alpha = fillStroke.fill.colour.GetAlpha();
	fore = bcolor(fillStroke.stroke.colour);
	fore.alpha = fillStroke.stroke.colour.GetAlpha();

	BitmapLock l(bitmap);
	ownerView->PushState();
	ownerView->SetDrawingMode(B_OP_ALPHA);
	ownerView->SetHighColor(back);
	ownerView->FillRect(rect);
	ownerView->SetPenSize(fillStroke.stroke.width);
	ownerView->SetHighColor(fore);
	ownerView->StrokeRect(rect);
	ownerView->PopState();
}


void SurfaceImpl::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops,
		GradientOptions options) {
	BRect rect(toBRect(rc));

	BGradientLinear gradient;
	switch(options) {
		case GradientOptions::leftToRight:
			gradient.SetStart(rect.LeftTop());
			gradient.SetEnd(rect.RightTop());
			break;
		case GradientOptions::topToBottom:
			gradient.SetStart(rect.LeftTop());
			gradient.SetEnd(rect.LeftBottom());
			break;
	}
	for (const ColourStop &stop : stops) {
		rgb_color color = bcolor(stop.colour);
		color.alpha = stop.colour.GetAlpha(); // FIXME
		gradient.AddColor(color, stop.position * 255.0f);
	}

	BitmapLock l(bitmap);
	ownerView->PushState();
	ownerView->SetDrawingMode(B_OP_ALPHA);
	ownerView->FillRect(rect, gradient);
	ownerView->PopState();
}

void SurfaceImpl::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	BRect dest(toBRect(rc));
	BRect src(0, 0, width, height);

	BBitmap* image = new BBitmap(src, B_RGBA32);
	image->SetBits((void*) pixelsImage, width * height, 0, B_RGBA32);

	{
		BitmapLock l(bitmap);
		ownerView->DrawBitmap(image, src, dest);
	}

	delete image;
}

void SurfaceImpl::Ellipse(PRectangle rc, FillStroke fillStroke) {
	BitmapLock l(bitmap);
	BRect rect(toBRect(rc));
	ownerView->SetHighColor(bcolor(fillStroke.fill.colour));
	ownerView->FillEllipse(rect);
	ownerView->SetPenSize(fillStroke.stroke.width);
	ownerView->SetHighColor(bcolor(fillStroke.stroke.colour));
	ownerView->StrokeEllipse(rect);
}

void SurfaceImpl::Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) {
	BRect rect = toBRect(rc);
	const XYPOSITION halfStroke = fillStroke.stroke.width / 2.0f;
	const XYPOSITION radius = rect.Height() / 2.0f - halfStroke;
	BRect rectInner = rect;
	rectInner.left += radius;
	rectInner.right -= radius;

	BShape path;

	const Ends leftSide = static_cast<Ends>(static_cast<int>(ends) & 0xf);
	const Ends rightSide = static_cast<Ends>(static_cast<int>(ends) & 0xf0);
	switch (leftSide) {
		case Ends::leftFlat:
			path.MoveTo(BPoint(rect.left + halfStroke, rect.top + halfStroke));
			path.LineTo(BPoint(rect.left + halfStroke, rect.bottom - halfStroke));
			break;
		case Ends::leftAngle:
			path.MoveTo(BPoint(rectInner.left + halfStroke, rect.top + halfStroke));
			path.LineTo(BPoint(rect.left + halfStroke, rect.top + radius + halfStroke));
			path.LineTo(BPoint(rectInner.left + halfStroke, rect.bottom - halfStroke));
			break;
		case Ends::semiCircles:
		default:
			path.MoveTo(BPoint(rectInner.left + halfStroke, rect.top + halfStroke));
			path.ArcTo(radius, radius, 0, false, true,
				BPoint(rectInner.left + halfStroke, rect.bottom - halfStroke));
			break;
	}

	switch (rightSide) {
		case Ends::rightFlat:
			path.LineTo(BPoint(rect.right - halfStroke, rect.bottom - halfStroke));
			path.LineTo(BPoint(rect.right - halfStroke, rect.top + halfStroke));
			break;
		case Ends::rightAngle:
			path.LineTo(BPoint(rectInner.right - halfStroke, rect.bottom - halfStroke));
			path.LineTo(BPoint(rect.right - halfStroke, rect.top + radius + halfStroke));
			path.LineTo(BPoint(rectInner.right - halfStroke, rect.top + halfStroke));
			break;
		case Ends::semiCircles:
		default:
			path.LineTo(BPoint(rectInner.right - halfStroke, rect.bottom - halfStroke));
			path.ArcTo(radius, radius, 0, false, true,
				BPoint(rectInner.right - halfStroke, rc.top + halfStroke));
			break;
	}

	path.Close();

	rgb_color back, fore;
	back = bcolor(fillStroke.fill.colour);
	back.alpha = fillStroke.fill.colour.GetAlpha();
	fore = bcolor(fillStroke.stroke.colour);
	fore.alpha = fillStroke.stroke.colour.GetAlpha();

	BitmapLock l(bitmap);
	ownerView->PushState();
	ownerView->MovePenTo(B_ORIGIN);
	ownerView->SetHighColor(back);
	ownerView->FillShape(&path);
	ownerView->SetPenSize(fillStroke.stroke.width);
	ownerView->SetHighColor(fore);
	ownerView->StrokeShape(&path);
	ownerView->PopState();
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceImpl& source = static_cast<SurfaceImpl &>(surfaceSource);

	BBitmap* image = source.GetBitmap();
	// If we could not get an image reference, fill the rectangle black
	if (image == NULL) {
		FillRectangle(rc, Fill(ColourRGBA(0)));
		return;
	}

	XYPOSITION imageW = image->Bounds().Width();
	XYPOSITION imageH = image->Bounds().Height();
	BRect srcRect = BRect(from.x, from.y, imageW - 1, imageH - 1);
	BRect destRect = toBRect(rc);
	BitmapLock l(bitmap);
	ownerView->DrawBitmap(image, srcRect, destRect);
}


std::unique_ptr<IScreenLineLayout> SurfaceImpl::Layout(const IScreenLine *screenLine) {
	return {};
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) {
	DrawTextNoClipUTF8(rc, font_, ybase, text, fore, back);
}

void SurfaceImpl::DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) {
	DrawTextClippedUTF8(rc, font_, ybase, text, fore, back);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore) {
	DrawTextTransparentUTF8(rc, font_, ybase, text, fore);
}

void SurfaceImpl::MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) {
	MeasureWidthsUTF8(font_, text, positions);
}

XYPOSITION SurfaceImpl::WidthText(const Font *font_, std::string_view text) {
	return WidthTextUTF8(font_, text);
}

void SurfaceImpl::DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) {
	BitmapLock l(bitmap);
	if (ownerView) {
		FillRectangle(rc, Fill(back));
		ownerView->SetFont(HFont(font_)->bfont);
		ownerView->SetHighColor(bcolor(fore.Opaque()));
		ownerView->SetLowColor(bcolor(back.Opaque()));
		BPoint where(rc.left, ybase);
		ownerView->DrawString(text.data(), text.length(), where);
	}
}

void SurfaceImpl::DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) {
	BitmapLock l(bitmap);
	if (ownerView) {
		FillRectangle(rc, Fill(back));
		ownerView->SetFont(HFont(font_)->bfont);
		ownerView->SetHighColor(bcolor(fore.Opaque()));
		ownerView->SetLowColor(bcolor(back.Opaque()));
		BPoint where(rc.left, ybase);
		ownerView->DrawString(text.data(), text.length(), where);
	}
}

void SurfaceImpl::DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore) {
	BitmapLock l(bitmap);
	if (ownerView) {
		ownerView->PushState();
		ownerView->SetHighColor(bcolor(fore));
		ownerView->SetDrawingMode(B_OP_OVER);
		ownerView->SetFont(HFont(font_)->bfont);
		ownerView->DrawString(text.data(), text.length(), BPoint(rc.left, ybase));
		ownerView->PopState();
	}
}

// borrowed from Qt port
void SurfaceImpl::MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) {
	BFont* font = HFont(font_)->bfont;
	if (font) {
		int fit = MultibyteToUTF8Length(text);
		int ui = 0;
		size_t i = 0;
		float currentPos = 0.0f;
		std::vector<float> escp(text.length());
		font->GetEscapements(text.data(), text.length(), escp.data());
		while(ui < fit) {
			size_t lenChar = UTF8CharLength(text.data() + i);
			int codeUnits = (lenChar < 4) ? 1 : 2;
			float pos = currentPos + escp[ui] * font->Size();
			for(unsigned int bytePos = 0; (bytePos < lenChar) && (i < text.length()); bytePos++) {
				positions[i++] = pos;
			}
			ui += codeUnits;
			currentPos = pos;
		}
		float lastPos = 0;
		if(i > 0)
			lastPos = positions[i - 1];
		while(i < text.length())
			positions[i++] = lastPos;
	}
}

XYPOSITION SurfaceImpl::WidthTextUTF8(const Font *font_, std::string_view text) {
	XYPOSITION width = round(HFont(font_)->bfont->StringWidth(text.data(), text.length()));
	return width;
}

XYPOSITION SurfaceImpl::Ascent(const Font *font_) {
	BFont* font = HFont(font_)->bfont;
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		return ceil(fh.ascent);
	}
	return 0;
}

XYPOSITION SurfaceImpl::Descent(const Font *font_) {
	BFont* font = HFont(font_)->bfont;
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		return ceil(fh.descent);
	}
	return 0;
}

XYPOSITION SurfaceImpl::InternalLeading(const Font *font_) {
	BFont* font = HFont(font_)->bfont;
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		return ceil(fh.leading);
	}
	return 0;
}

XYPOSITION SurfaceImpl::Height(const Font *font_) {
	BFont* font = HFont(font_)->bfont;
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		int h = (ceil(fh.descent) + ceil(fh.ascent));
		return h;
	}
	return 0;
}

// This string contains a good range of characters to test for size.
const char sizeString[] = "`~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890"
						  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

XYPOSITION SurfaceImpl::AverageCharWidth(const Font *font_) {
	std::string_view sv(sizeString);
	return (XYPOSITION) ((WidthText(font_, sv) / (float) sv.length()) + 0.5);
}

void SurfaceImpl::SetClip(PRectangle rc) {
	BRegion r(toBRect(rc));
	BitmapLock l(bitmap);
	ownerView->PushState();
	ownerView->ConstrainClippingRegion(&r);
}

void SurfaceImpl::PopClip() {
	BitmapLock l(bitmap);
	ownerView->PopState();
}

void SurfaceImpl::FlushCachedState() {
	BitmapLock l(bitmap);
	ownerView->Flush();
}

void SurfaceImpl::FlushDrawing() {
	BitmapLock l(bitmap);
	ownerView->Sync();
}

std::unique_ptr<Surface> Surface::Allocate(Technology) {
	return std::make_unique<SurfaceImpl>();
}

Window::~Window() noexcept {
}

void Window::Destroy() noexcept {
	if(wid == nullptr) return;

	BView *view = bview(wid);
	if(view != nullptr) {
		BLooper* looper = view->Looper();
		if(looper->Lock())
		{
			view->RemoveSelf();
			looper->Unlock();
			delete view;
			wid = 0;
		}
	} else {
		BWindow* window = bwindow(wid);
		if(window->LockLooper()) {
			wid = 0;
			window->Quit();
		}
	}
}

PRectangle Window::GetPosition() const {
	PRectangle rc(0, 0, 1, 1);

	// The frame rectangle gives the position of this view inside the parent view
	if (wid) {
		BView *view = bview(wid);
		if(view != nullptr) {
			rc = toPRect(view->ConvertToScreen(view->Bounds()));
		} else {
			BWindow* window = bwindow(wid);
			BAutolock a(window->Looper());
			if(a.IsLocked()) {
				rc = toPRect(window->Frame());
			}
		}
	}

	return rc;
}

void Window::SetPosition(PRectangle rc) {
	if(wid == nullptr) return;

	BView *view = bview(wid);
	if(view != nullptr) {
		BAutolock a(view->Looper());
		if(a.IsLocked()) {
			view->MoveTo(BPoint(rc.left, rc.top));
			view->ResizeTo(rc.Width(), rc.Height());
		}
	} else {
		BWindow* window = bwindow(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			const float topOffset = window->Frame().top - window->DecoratorFrame().top;
			const float leftOffset = window->Frame().left - window->DecoratorFrame().left;
			window->MoveTo(BPoint(rc.left + leftOffset, rc.top + topOffset));
			window->ResizeTo(rc.Width(), rc.Height());
		}
	}
}

void Window::SetPositionRelative(PRectangle rc, const Window* window) {
	if(wid == nullptr) return;

	PRectangle origin = window->GetPosition();
	rc.Move(origin.left, origin.top);
	SetPosition(rc);
}

PRectangle Window::GetClientPosition() const {
	return GetPosition();
}

void Window::Show(bool show) {
	if (wid == nullptr) return;

	BView* view = bview(wid);
	if(view != nullptr) {
		show ? view->Show() : view->Hide();
	} else {
		BWindow* window = bwindow(wid);
		show ? window->Show() : window->Hide();
	}
}

void Window::InvalidateAll() {
	if(wid == nullptr) return;

	BView *view = bview(wid);
	if(view != nullptr) {
		BAutolock a(view->Looper());
		if(a.IsLocked()) {
			int count = view->CountChildren();
			for (int i = 0; i < count; i++)
				view->ChildAt(i)->Invalidate();
		}
	} else {
		BWindow* window = bwindow(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			int count = window->CountChildren();
			for (int i = 0; i < count; i++)
				window->ChildAt(i)->Invalidate();
		}
	}
}

void Window::InvalidateRectangle(PRectangle rc) {
	if(wid == nullptr) return;

	BView *view = bview(wid);
	if(view != nullptr) {
		BAutolock a(view->Looper());
		if(a.IsLocked()) {
			int count = view->CountChildren();
			for (int i = 0; i < count; i++)
				view->ChildAt(i)->Invalidate(toBRect(rc));
		}
	} else {
		BWindow* window = bwindow(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			int count = window->CountChildren();
			for (int i = 0; i < count; i++)
				window->ChildAt(i)->Invalidate(toBRect(rc));
		}
	}
}

void Window::SetCursor(Cursor curs) {
	static BCursor upArrow(B_CURSOR_ID_RESIZE_NORTH);
	static BCursor horizArrow(B_CURSOR_ID_RESIZE_EAST_WEST);
	static BCursor vertArrow(B_CURSOR_ID_RESIZE_NORTH_SOUTH);
	static BCursor wait(B_CURSOR_ID_PROGRESS);
	static BCursor hand(B_CURSOR_ID_FOLLOW_LINK);
	if (curs == cursorLast)
		return;
	if (wid) {
		const BCursor* cursor = B_CURSOR_SYSTEM_DEFAULT;
		switch ( curs ) {
			case Cursor::text:
				cursor = B_CURSOR_I_BEAM;
				break;
			case Cursor::wait:
				cursor = &wait;
				break;
			case Cursor::horizontal:
				cursor = &horizArrow;
				break;
			case Cursor::vertical:
				cursor = &vertArrow;
				break;
			case Cursor::up:
				cursor = &upArrow;
				break;
			case Cursor::hand:
				cursor = &hand;
				break;
			case Cursor::arrow:
			case Cursor::reverseArrow:
			case Cursor::invalid:
			default:
				cursor = B_CURSOR_SYSTEM_DEFAULT;
				break;
		}
		// this function is only used with wMain so we can get away with not
		// checking if wid is BWindow or BView
		BAutolock a(bview(wid)->Looper());
		if(a.IsLocked()) {
			bview(wid)->ChildAt(0)->SetViewCursor(cursor);
		}
		cursorLast = curs;
	}
}

PRectangle Window::GetMonitorRect(Point) {
	// TODO Haiku currently doesn't support multiple screens so we
	//      just return first screen rect
	BScreen screen(0);
	BRect rect = screen.Frame();
	return PRectangle(rect.left, rect.top, rect.right, rect.bottom);
}

Menu::Menu() noexcept : mid(0) {}

void Menu::CreatePopUp() {
	Destroy();
	mid = new BPopUpMenu("popupMenu", false, false);
}

void Menu::Destroy() noexcept {
	if (mid != 0) {
		BPopUpMenu *menu = static_cast<BPopUpMenu *>(mid);
		delete menu;
	}
	mid = 0;
}

void Menu::Show(Point pt, const Window &window) {
	BPopUpMenu *menu = static_cast<BPopUpMenu *>(mid);
	menu->Go(BPoint(pt.x, pt.y), true);
	Destroy();
}

class ListBoxImpl : public ListBox {
public:
	ListBoxImpl();
	~ListBoxImpl();

	void SetFont(const Font *font);
	void Create(Window &parent, int ctrlID, Point location, int lineHeight_, bool unicodeMode_, Technology technology_);
	void SetAverageCharWidth(int width);
	void SetVisibleRows(int rows);
	int GetVisibleRows() const;
	PRectangle GetDesiredRect();
	int CaretFromEdge();
	void Clear() noexcept;
	void Append(char *s, int type = -1);
	int Length();
	void Select(int n);
	int GetSelection();
	int Find(const char *prefix);
	std::string GetValue(int n);
	void RegisterImage(int type, const char *xpm_data);
	void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage);
	void ClearRegisteredImages();
	void SetDelegate(IListBoxDelegate *);
	void SetList(const char* list, char separator, char typesep);
	void SetOptions(ListOptions options_);

private:
	BWindow* listWindow;
	int visibleRows;
	int maxTextWidth;

	class BitmapSet {
	public:
		BitmapSet() : maxWidth(-1), maxHeight(-1) {}

		void Clear() noexcept;
		void AddImage(int ident, std::shared_ptr<BBitmap> bitmap);
		std::shared_ptr<BBitmap> Get(int ident);
		int GetWidth() const noexcept;
		int GetHeight() const noexcept;
	private:
		std::unordered_map<int, std::shared_ptr<BBitmap>> images;
		int maxWidth;
		int maxHeight;
	};

	BitmapSet images;

	class CustomScrollView : public BScrollView {
	public:
		CustomScrollView(const char* name, BView* target);

		virtual void DoLayout();
	};

	class ListBoxView : public BListView {
	public:
		ListBoxView(const char* name);

		void SetDelegate(IListBoxDelegate* lbDelegate);
		virtual void MessageReceived(BMessage* message);
		virtual void SelectionChanged();

	private:
		IListBoxDelegate* delegate;
	};

	class ListBoxWindow : public BWindow {
	public:
		ListBoxWindow(BRect rect, BWindow* parent, BMessenger messenger);

		ListBoxView* GetListView() { return listView; }
		void SetIconSize(BSize size) { iconSize = size; }
		BSize IconSize() const { return iconSize; }
		void NotifyParent(BMessage* message) { parentMessenger.SendMessage(message); }

	private:
		CustomScrollView* scrollView;
		ListBoxView* listView;
		BSize iconSize;
		BMessenger parentMessenger;
	};

	class ImageItem : public BStringItem {
	public:
		ImageItem(const char* text, std::shared_ptr<BBitmap> bitmap = nullptr);
		virtual ~ImageItem();

		virtual void DrawItem(BView* owner, BRect frame, bool complete = false);
		virtual void Update(BView* owner, const BFont* font);
	private:
		std::shared_ptr<BBitmap> image;
	};

	friend ListBoxImpl::ListBoxView* blist(WindowID wid);
};

void ListBoxImpl::BitmapSet::Clear() noexcept {
	images.clear();
	maxWidth = -1;
	maxHeight = -1;
}

void ListBoxImpl::BitmapSet::AddImage(int ident, std::shared_ptr<BBitmap> image)
{
	BRect rect = image->Bounds();
	maxWidth = std::fmax(maxWidth, rect.Width());
	maxHeight = std::fmax(maxHeight, rect.Height());
	images[ident] = std::move(image);
}

std::shared_ptr<BBitmap> ListBoxImpl::BitmapSet::Get(int ident) {
	auto it = images.find(ident);
	if(it != images.end()) {
		return it->second;
	}
	return nullptr;
}

int ListBoxImpl::BitmapSet::GetWidth() const noexcept {
	if(maxWidth < 0) {
		return 0;
	}
	return maxWidth;
}

int ListBoxImpl::BitmapSet::GetHeight() const noexcept {
	if(maxHeight < 0) {
		return 0;
	}
	return maxHeight;
}

ListBoxImpl::CustomScrollView::CustomScrollView(const char* name, BView* target)
	:
	BScrollView(name, target, 0, false, true, B_NO_BORDER)
{
}

void ListBoxImpl::CustomScrollView::DoLayout()
{
	BScrollView::DoLayout();

	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	scrollBar->ResizeBy(0, 2);
	scrollBar->MoveBy(0, -1);
}

ListBoxImpl::ListBoxView::ListBoxView(const char* name)
	: BListView(name, B_SINGLE_SELECTION_LIST), delegate(NULL)
{
}

void
ListBoxImpl::ListBoxView::SetDelegate(IListBoxDelegate* lbDelegate)
{
	delegate = lbDelegate;
}

void
ListBoxImpl::ListBoxView::MessageReceived(BMessage* message)
{
	if(message->what == B_MOUSE_DOWN && message->GetInt32("clicks", 0) == 2) {
		if(delegate) {
			// We must send a message to main view, to have ListNotify processed
			// in main thread. Double click quits the list box window, and kills
			// the thread, so if it was running in ListBoxWindow thread,
			// ListNotify would be cut halfway, and text would not be inserted.
			BMessage msg('lbdc');
			msg.AddPointer("delegate", delegate);
			static_cast<ListBoxWindow*>(Window())->NotifyParent(&msg);
		}
	}

	BListView::MessageReceived(message);
}

void
ListBoxImpl::ListBoxView::SelectionChanged()
{
	if(delegate) {
		ListBoxEvent event(ListBoxEvent::EventType::selectionChange);
		delegate->ListNotify(&event);
	}
}

ListBoxImpl::ListBoxWindow::ListBoxWindow(BRect rect, BWindow* parent, BMessenger messenger)
	:
	BWindow(rect, "autocomplete", B_MODAL_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS),
		iconSize(BSize(0, 0)),
	parentMessenger(messenger)
{
	AddToSubset(parent);

	listView = new ListBoxView("ListView");
	scrollView = new CustomScrollView("ListBoxScrollView", listView);

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);
	layout->AddView(scrollView);
	layout->SetInsets(0.f, 0.f, -1.0f, 0.f);
}

ListBoxImpl::ListBoxView* blist(WindowID wid) {
	return static_cast<ListBoxImpl::ListBoxView*>(
		static_cast<ListBoxImpl::ListBoxWindow*>(wid)->GetListView());
}

ListBoxImpl::ImageItem::ImageItem(const char* text, std::shared_ptr<BBitmap> bitmap)
	: BStringItem(text), image(bitmap)
{
}

ListBoxImpl::ImageItem::~ImageItem()
{
}

void
ListBoxImpl::ImageItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	float spacing = be_control_look->DefaultLabelSpacing();

	rgb_color lowColor = owner->LowColor();

	if(IsSelected() || complete) {
		if(IsSelected())
			owner->SetLowUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			owner->SetLowUIColor(owner->ViewUIColor());
		owner->FillRect(frame, B_SOLID_LOW);
	} else
		owner->SetLowUIColor(owner->ViewUIColor());

	if(image != nullptr) {
		owner->DrawBitmap(image.get(), BPoint(frame.left + spacing, frame.top + spacing));
	}

	auto window = static_cast<ListBoxWindow*>(owner->Window());
	if(window->IconSize() != BSize(0, 0)) {
		owner->MovePenTo(frame.left + window->IconSize().width + spacing * 2, frame.top + BaselineOffset());
	} else {
		owner->MovePenTo(frame.left + spacing, frame.top + BaselineOffset());
	}

	if(Text() != nullptr) {
		owner->DrawString(Text());
	}

	owner->SetLowColor(lowColor);
}

void
ListBoxImpl::ImageItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);

	if(image != nullptr) {
		float spacing = be_control_look->DefaultLabelSpacing() * 2;
		float imageWidth = image->Bounds().Width() + spacing;
		float imageHeight = image->Bounds().Height() + spacing;
		SetWidth(Width() + imageWidth);
		SetHeight(std::fmax(Height(), imageHeight));
	}
}

ListBox::ListBox() noexcept {}
ListBox::~ListBox() {}

std::unique_ptr<ListBox> ListBox::Allocate() {
	return std::make_unique<ListBoxImpl>();
}

ListBoxImpl::ListBoxImpl()
	:
	visibleRows(5), maxTextWidth(0)
{
}
ListBoxImpl::~ListBoxImpl(){}

void
ListBoxImpl::SetFont(const Font *font)
{
	BListView* list = blist(wid);
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		list->SetFont(HFont(font)->bfont);
	}
}

void
ListBoxImpl::Create(Window &parent, int ctrlID, Point location, int lineHeight_, bool unicodeMode_, Technology technology_)
{
	BView* parentView = bview(parent.GetID());
	BWindow* parentWindow;
	BMessenger messenger;
	if(parentView != nullptr) {
		parentWindow = parentView->Window();
		messenger = BMessenger(parentView->ChildAt(0));
	} else {
		parentWindow = bwindow(parent.GetID());
		messenger = BMessenger(parentWindow);
	}
	listWindow = new ListBoxWindow(BRect(location.x, location.y, location.x + 50, location.y + 50), parentWindow, messenger);
	wid = listWindow;
}

void
ListBoxImpl::SetAverageCharWidth(int width)
{

}

void ListBoxImpl::SetVisibleRows(int rows)
{
	visibleRows = rows;
}

int ListBoxImpl::GetVisibleRows() const
{
	return visibleRows;
}

PRectangle
ListBoxImpl::GetDesiredRect() {
	BListView* list = blist(wid);

	int rows = Length();
	if (rows == 0 || rows > visibleRows) {
		rows = visibleRows;
	}
	int rowHeight = list->ItemAt(0)->Height();
	int height = (rows * rowHeight) - 1;
	int width = maxTextWidth + (2 * be_control_look->DefaultLabelSpacing());
	auto window = static_cast<ListBoxWindow*>(list->Window());
	width += B_V_SCROLL_BAR_WIDTH;
	width += window->IconSize().Width() + be_control_look->DefaultLabelSpacing();
	return PRectangle(0, 0, width, height);
}

int
ListBoxImpl::CaretFromEdge() {
	const int pixWidth = static_cast<ListBoxWindow*>(listWindow)->IconSize().width;
	const int itemInset = be_control_look->DefaultLabelSpacing();
	const int decorOffset = listWindow->Frame().left - listWindow->DecoratorFrame().left;
	if(pixWidth == 0) {
		return decorOffset + itemInset;
	} else {
		return decorOffset + itemInset + pixWidth + itemInset;
	}
}

void
ListBoxImpl::Clear() noexcept {
	BListView* list = blist(wid);
	BListItem* item;
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		while((item = list->RemoveItem((int32)0)) != NULL) {
			delete item;
		}
	}
}

void
ListBoxImpl::Append(char *s, int type) {
	BListView* list = blist(wid);
	ImageItem* item = new ImageItem(s, type == -1 ? nullptr : images.Get(type));
	int sWidth = list->StringWidth(s);
	if(sWidth > maxTextWidth) maxTextWidth = sWidth;

	auto window = static_cast<ListBoxWindow*>(list->Window());
	if(window->IconSize() == BSize(0, 0) && type != -1) {
		window->SetIconSize(BSize(images.GetWidth(), images.GetHeight()));
	}

	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		list->AddItem(item);
	}
}

int
ListBoxImpl::Length(){
	BListView* list = blist(wid);
	return list->CountItems();
}

void
ListBoxImpl::Select(int n){
	BListView* list = blist(wid);
	list->Select(n);
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		list->ScrollToSelection();
	}
}

int
ListBoxImpl::GetSelection(){
	BListView* list = blist(wid);
	return list->CurrentSelection();
}

int
ListBoxImpl::Find(const char *prefix){
	// TODO
	return 0;
}

std::string
ListBoxImpl::GetValue(int n) {
	BListView* list = blist(wid);
	ImageItem* item = static_cast<ImageItem*>(list->ItemAt(n));
	return std::string(item->Text());
}

void
ListBoxImpl::RegisterImage(int type, const char *xpm_data){
	XPM xpmImage(xpm_data);
	RGBAImage rgbaImage(xpmImage);
	BRect rect(0, 0, rgbaImage.GetWidth(), rgbaImage.GetHeight());
	auto bitmap = std::make_shared<BBitmap>(rect, B_RGBA32);
	bitmap->SetBits(rgbaImage.Pixels(), rgbaImage.CountBytes(), 0, B_RGBA32);
	images.AddImage(type, std::move(bitmap));
}

void
ListBoxImpl::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage){
	BRect rect(0, 0, width - 1, height - 1);
	auto bitmap = std::make_shared<BBitmap>(rect, B_RGBA32);
	bitmap->SetBits(pixelsImage, width * height * 4, 0, B_RGBA32);
	images.AddImage(type, std::move(bitmap));
}

void
ListBoxImpl::ClearRegisteredImages(){
	images.Clear();
}

void
ListBoxImpl::SetDelegate(IListBoxDelegate* lbDelegate) {
	blist(wid)->SetDelegate(lbDelegate);
}

void
ListBoxImpl::SetList(const char* list, char separator, char typesep){
	// This method is *not* platform dependent.
	// It is borrowed from the GTK implementation.
	Clear();
	size_t count = strlen(list) + 1;
	std::vector<char> words(list, list+count);
	char *startword = &words[0];
	char *numword = NULL;
	int i = 0;
	for (; words[i]; i++) {
		if (words[i] == separator) {
			words[i] = '\0';
			if (numword)
				*numword = '\0';
			Append(startword, numword?atoi(numword + 1):-1);
			startword = &words[0] + i + 1;
			numword = NULL;
		} else if (words[i] == typesep) {
			numword = &words[0] + i;
		}
	}
	if (startword) {
		if (numword)
			*numword = '\0';
		Append(startword, numword?atoi(numword + 1):-1);
	}
}

void ListBoxImpl::SetOptions(ListOptions options_) {
}

ColourRGBA Platform::Chrome() {
	rgb_color c = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	return ColourRGBA(c.red, c.green, c.blue);
}

ColourRGBA Platform::ChromeHighlight() {
	rgb_color c = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	return ColourRGBA(c.red, c.green, c.blue);
}

char* default_font_family = NULL;

const char *Platform::DefaultFont() {
	if(default_font_family == NULL) {
		if(be_plain_font) {
			font_family ff;
			font_style fs;
			be_plain_font->GetFamilyAndStyle(&ff, &fs);
			default_font_family = strdup(ff);
		} else {
			default_font_family = strdup("Sans");
		}
	}
	return default_font_family;
}

int Platform::DefaultFontSize() {
	return be_plain_font->Size();
}

unsigned int Platform::DoubleClickTime() {
	bigtime_t interval;
	if(get_click_speed(&interval) == B_OK)
		return interval / 1000;
	return 500;
}

#ifdef TRACE

void Platform::DebugDisplay(const char *s) noexcept {
	fprintf( stderr, s );
}

void Platform::DebugPrintf(const char *format, ...) noexcept {
	const int BUF_SIZE = 2000;
	char buffer[BUF_SIZE];

	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, BUF_SIZE, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}

#else

void Platform::DebugDisplay(const char *) noexcept {}

void Platform::DebugPrintf(const char *, ...) noexcept {}

#endif

static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) noexcept {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) noexcept {
	char buffer[2000];
	sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	strcat(buffer, "\r\n");
	Platform::DebugDisplay(buffer);
}

}
