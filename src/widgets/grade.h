#ifndef __GPX2VIDEO__WIDGETS__GRADE_H__
#define __GPX2VIDEO__WIDGETS__GRADE_H__

#include "log.h"
#include "videowidget.h"


class GradeWidget : public VideoWidget {
public:
	virtual ~GradeWidget() {
		log_call();
	}

	static GradeWidget * create(GPX2Video &app) {
		GradeWidget *widget;

		log_call();

		widget = new GradeWidget(app, "grade");

		return widget;
	}

	void render(OIIO::ImageBuf *buf, const GPXData &data) {
		char s[128];

		const int w = 150;

		double divider = (double) (this->height() - (2 * this->border())) / (double) w;

		sprintf(s, "%.0f%%", data.grade());

		this->fillBackground(buf);
		this->add(buf, this->x(), this->y(), "./assets/picto/DataOverlay_icn_grade.png", 
				label().c_str(), s, divider);
	}

private:
	GradeWidget(GPX2Video &app, std::string name)
		: VideoWidget(app, name) {
	}
};

#endif

