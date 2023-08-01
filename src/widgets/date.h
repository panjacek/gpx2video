#ifndef __GPX2VIDEO__WIDGETS__DATE_H__
#define __GPX2VIDEO__WIDGETS__DATE_H__

#include "log.h"
#include "videowidget.h"


class DateWidget : public VideoWidget {
public:
	virtual ~DateWidget() {
		log_call();

		if (bg_buf_)
			delete bg_buf_;

		if (fg_buf_)
			delete fg_buf_;
	}

	static DateWidget * create(GPX2Video &app) {
		DateWidget *widget;

		log_call();

		widget = new DateWidget(app, "date");

		widget->setFormat("%Y-%m-%d");

		return widget;
	}

	OIIO::ImageBuf * prepare(void) {
		int x, y;

		bool with_picto = this->hasFlag(VideoWidget::FlagPicto);

		if (bg_buf_ == NULL) {
			x = this->padding(VideoWidget::PaddingLeft);
			x += (with_picto) ? this->height() : 0;
			y = 0;

			this->createBox(&bg_buf_, this->width(), this->height());
			this->drawBorder(bg_buf_);
			this->drawBackground(bg_buf_);
			if (with_picto)
				this->drawImage(bg_buf_, this->border(), this->border(), "./assets/picto/DataOverlay_icn_date.png", VideoWidget::ZoomFit);
			this->drawLabel(bg_buf_, x, y, label().c_str());
		}

		return bg_buf_;
	}

	OIIO::ImageBuf * render(const GPXData &data) {
		int x, y;

		char s[128];

		struct tm time;

		bool with_picto = this->hasFlag(VideoWidget::FlagPicto);

		(void) data;

		// Refresh dynamic info
		if ((fg_buf_ != NULL) && (data.type() == GPXData::TypeUnchanged))
			goto skip;

		// Format data
		// Don't use gps time, but camera time!
		// Indeed, with garmin devices, gpx time has an offset.
		localtime_r(&app_.time(), &time);

		strftime(s, sizeof(s), format().c_str(), &time);

		// Compute text position
		x = this->padding(VideoWidget::PaddingLeft);
		x += (with_picto) ? this->height() : 0;
		y = 0;

		// Refresh dynamic info
		if (fg_buf_ != NULL)
			delete fg_buf_;

		this->createBox(&fg_buf_, this->width(), this->height());
		this->drawValue(fg_buf_, x, y, s);

skip:
		return fg_buf_;
	}

private:
	OIIO::ImageBuf *bg_buf_;
	OIIO::ImageBuf *fg_buf_;

	DateWidget(GPX2Video &app, std::string name)
		: VideoWidget(app, name) 
   		, bg_buf_(NULL)
   		, fg_buf_(NULL) {
	}
};

#endif

