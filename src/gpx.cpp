#include <fstream>
#include <iostream>
#include <string>

#include <string.h>
//#define __USE_XOPEN  // For strptime
#include <time.h>
#include <math.h>

#include "utmconvert/utmconvert.h"

#include "gpxlib/Parser.h"
#include "gpxlib/ReportCerr.h"

#include "log.h"
#include "gpx.h"


void show(gpx::Node &node, unsigned width)
{
	std::string value(width, ' ');

  value = node.getValue();

  if (value.length() > width)
  {
    value = value.substr(0, width);
  }
  else if (value.length() < width)
  {
    value.insert(0, width - value.length(), ' ');
  }

  std::cout << value << ' ';
}



#define NOISE 0.1


// GPX Data Point
//----------------

GPXData::GPXData() 
	: nbr_points_(0)
	, valid_(false)
	, elapsedtime_(0)
	, duration_(0)
	, distance_(0)
	, speed_(0) 
	, maxspeed_(0)
	, avgspeed_(0)
	, grade_(0) 
	, cadence_(0) {

	nbr_predictions_ = 0;

	filter_ = alloc_filter_velocity2d(10.0);
}


GPXData::~GPXData() {
//	free_filter(filter_);
}


void GPXData::dump(void) {
	char s[128];

	struct tm time;

	localtime_r(&cur_pt_.time, &time);

	strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &time);

	printf("  GPX Time: %s Distance: %.3f km in %d seconds, current speed is %.3f (valid: %s)\n",
		s,
		distance_/1000.0, elapsedtime_, speed_, 
		valid() ? "true" : "false");  
}


void GPXData::convert(struct GPXData::point *pt, gpx::WPT *wpt) {
	const char *s;

	struct tm time;

	struct Utm_val utm;

	pt->valid = true;

	// Convert time - GPX file contains UTC time
	s = wpt->time().getValue().c_str();

	// Try format: "2020:12:13 08:55:48.215"
	memset(&time, 0, sizeof(time));
	if (strptime(s, "%Y:%m:%d %H:%M:%S.", &time) != NULL)
		pt->time = timegm(&time);
	// Try format: "2020-07-28T07:04:43.000Z"
	else if (strptime(s, "%Y-%m-%dT%H:%M:%S.", &time) != NULL)
		pt->time = timegm(&time);
	// Try format: "2020-07-28T07:04:43Z"
	else if (strptime(s, "%Y-%m-%dT%H:%M:%SZ", &time) != NULL)
		pt->time = timegm(&time);
	else
		pt->valid = false;

	// Convert lat, lon & ele
	pt->lat = (double) wpt->lat();
	pt->lon = (double) wpt->lon();
	pt->ele = (double) wpt->ele();

	utm = to_utm(pt->lat, pt->lon);

	// Return result
	pt->x = utm.x;
	pt->y = utm.y;
}

#include <GeographicLib/Geodesic.hpp>

bool GPXData::compute(void) {
//	double dx = (cur_pt_.x - prev_pt_.x);
//	double dy = (cur_pt_.y - prev_pt_.y);
	double dz = (cur_pt_.ele - prev_pt_.ele);

//	double dc = sqrt(dx*dx + dy*dy + dz*dz);

	double dc;

	double speed = 0;

	GeographicLib::Math::real lat1 = cur_pt_.lat;
	GeographicLib::Math::real lon1 = cur_pt_.lon;
	GeographicLib::Math::real lat2 = prev_pt_.lat;
	GeographicLib::Math::real lon2 = prev_pt_.lon;
	GeographicLib::Math::real d;

	GeographicLib::Geodesic gsic(6378388, 1/297.0);
	gsic.Inverse(lat1, lon1, lat2, lon2,
		d);

	dc = d;

	double dt = difftime(cur_pt_.time, prev_pt_.time);

	duration_ += dt;
	distance_ += dc;
	if (dt > 0)
		speed = (3600.0 * dc) / (1000.0 * dt);
	if (abs((int) (speed - speed_)) < 50)
		speed_ = speed;
	if (speed_ > maxspeed_)
		maxspeed_ = speed_;
	avgspeed_ = (3600.0 * distance_) / (1000.0 * duration_);

//	dc = sqrt(dx*dx + dy*dy);

	grade_ = 100 * dz / dc;

	return true;
}


void GPXData::init(void) {
	memcpy(&cur_pt_, &next_pt_, sizeof(cur_pt_));
	memcpy(&prev_pt_, &cur_pt_, sizeof(prev_pt_));
}


void GPXData::predict(enum TelemetrySettings::Filter filter) {
	struct Utm_val utm;

	struct point prev_pt;

	nbr_predictions_++;

	// Save previous point
	memcpy(&prev_pt, &prev_pt_, sizeof(prev_pt));
	memcpy(&prev_pt_, &cur_pt_, sizeof(prev_pt_));

	cur_pt_.time += 1; // Prediction should occur each 1 second

	switch (filter) {
	case TelemetrySettings::FilterKalman:
		// Predict
		::update(filter_);
		::get_lat_long(filter_, &cur_pt_.lat, &cur_pt_.lon);
		break;

	case TelemetrySettings::FilterLinear:
		cur_pt_.lat += (prev_pt_.lat - prev_pt.lat);
		cur_pt_.lon += (prev_pt_.lon - prev_pt.lon);
		cur_pt_.ele += (prev_pt_.ele - prev_pt.ele);
		break;

	case TelemetrySettings::FilterInterpolate:
		cur_pt_.lat += (next_pt_.lat - prev_pt_.lat) / (next_pt_.time - prev_pt_.time);
		cur_pt_.lon += (next_pt_.lon - prev_pt_.lon) / (next_pt_.time - prev_pt_.time);
		cur_pt_.ele += (next_pt_.ele - prev_pt_.ele) / (next_pt_.time - prev_pt_.time);
		break;

	case TelemetrySettings::FilterNone:
	default:
		break;
	}

	if (filter != TelemetrySettings::FilterNone) {
		utm = to_utm(cur_pt_.lat, cur_pt_.lon);

		cur_pt_.x = utm.x;
		cur_pt_.y = utm.y;

		compute();
	}

	elapsedtime_ += (cur_pt_.time - prev_pt_.time);

	return;
}


void GPXData::update(enum TelemetrySettings::Filter filter) {
	memcpy(&prev_pt_, &cur_pt_, sizeof(prev_pt_));

	switch (filter) {
	case TelemetrySettings::FilterKalman:
		// Measure
		memcpy(&cur_pt_, &next_pt_, sizeof(cur_pt_));

		// Correct
		::update_velocity2d(filter_, next_pt_.lat, next_pt_.lon, 1.0);

		// Filter
		::get_lat_long(filter_, &cur_pt_.lat, &cur_pt_.lon);
		break;
	
	case TelemetrySettings::FilterLinear:
	case TelemetrySettings::FilterInterpolate:
	case TelemetrySettings::FilterNone:
	default:
		memcpy(&cur_pt_, &next_pt_, sizeof(cur_pt_));

		break;
	}

	elapsedtime_ += (cur_pt_.time - prev_pt_.time);

	if (filter == TelemetrySettings::FilterNone)
		prev_pt_.time -= nbr_predictions_;

	compute();

	nbr_predictions_ = 0;
}


void GPXData::read(gpx::WPT *wpt) {
	struct point pt;

	convert(&pt, wpt);

	// Skip point if not valid
	if (!pt.valid)
		return;

	// Extensions
	gpx::Node *extensions = wpt->extensions().getElements().front();

	if (extensions) {
		for (std::list<gpx::Node*>::const_iterator iter = extensions->getElements().begin(); 
			iter != extensions->getElements().end(); ++iter) {
			gpx::Node *node = (*iter);

			std::string name = node->getName();

			if (name.find("atemp") != std::string::npos)
				temperature_ = std::stod(node->getValue());
			else if (name.find("cad") != std::string::npos)
				cadence_ = std::stoi(node->getValue());
			else if (name.find("hr") != std::string::npos)
				heartrate_ = std::stoi(node->getValue());
		}
	}

	// Save point
	memcpy(&next_pt_, &pt, sizeof(next_pt_));

	valid_ = true;
}


// GPX File Reader
//-----------------

GPX::GPX(std::ifstream &stream, gpx::GPX *root, enum TelemetrySettings::Filter filter) 
	: stream_(stream)
	, root_(root)
	, offset_(0) 
	, filter_(filter) {
}


GPX::~GPX() {
}


GPX * GPX::open(const std::string &filename, enum TelemetrySettings::Filter filter) {
	GPX *gpx = NULL;

	GPXData data;

	gpx::GPX *root;
//	gpx::ReportCerr report;
	gpx::Parser parser(NULL); //&report);

    std::ifstream stream = std::ifstream(filename);

	if (!stream.is_open()) {
		log_error("Open '%s' GPX file failure, please check that file is readable", filename.c_str());
		goto failure;
	}

	root = parser.parse(stream);

	if (root == NULL) {
		log_error("Parsing of '%s' failed due to %s on line %d and column %d", 
			filename.c_str(), parser.errorText().c_str(),
			parser.errorLineNumber(), parser.errorColumnNumber());
		goto failure;
	}

	gpx = new GPX(stream, root, filter);

	// Parse activity start time
	gpx->parse();
	gpx->retrieveFirst(data);

	gpx->start_activity_ = data.time();

failure:
	return gpx;
}


void GPX::dump(void) {
	std::cout << "Track info:" << std::endl;
	std::cout << "  Name        : " << trk_->name()  .getValue() << std::endl;
	std::cout << "  Comment     : " << trk_->cmt()   .getValue() << std::endl;
	std::cout << "  Description : " << trk_->desc()  .getValue() << std::endl;
	std::cout << "  Source      : " << trk_->src()   .getValue() << std::endl;
	std::cout << "  Type        : " << trk_->type()  .getValue() << std::endl;
	std::cout << "  Number      : " << trk_->number().getValue() << std::endl;
	std::cout << "  Segments:   : " << trk_->trksegs().list().size()  << std::endl;
}


bool GPX::parse(void) {
	std::list<gpx::TRK*> &trks = root_->trks().list();

	for (std::list<gpx::TRK *>::iterator node = trks.begin(); node != trks.end(); ++node) {
		gpx::TRK *trk = (*node);
         
		if (trk == nullptr)
 			continue;

		trk_ = trk;

		// Parse only the first track
		break;
	}

	return true;
}


void GPX::setStartTime(time_t start_time) {
	start_time_ = start_time;
}


void GPX::setStartTime(struct tm *start_time) {
	time_t t = timegm(start_time);

	setStartTime(t);
}


void GPX::setStartTime(char *start_time) {
	struct tm time;

	memset(&time, 0, sizeof(time));

	strptime(start_time, "%Y:%m:%d %H:%M:%S.", &time);

	setStartTime(&time);
}


int GPX::timeOffset(void) const {
	return offset_;
}


void GPX::setTimeOffset(const int& offset) {
	offset_ = offset;
}


enum GPX::Data GPX::retrieveFirst(GPXData &data) {
	gpx::WPT *wpt;
	std::list<gpx::TRKSeg*> &trksegs = trk_->trksegs().list();

	data = GPXData();

	iter_seg_ = trksegs.begin();
	
	if (iter_seg_ != trksegs.end()) {
		gpx::TRKSeg *trkseg = (*iter_seg_);

		std::list<gpx::WPT*> &trkpts = trkseg->trkpts().list();
		iter_pts_ = trkpts.begin();

		if (iter_pts_ != trkpts.end()) {
			wpt = (*iter_pts_);

			data.read(wpt);
			data.init();

			return GPX::DataMeasured;
		}
	}

	return GPX::DataEof;
}


enum GPX::Data GPX::retrieveNext(GPXData &data, int timecode_ms) {
	enum GPX::Data result = GPX::DataEof;

	int timestamp = start_time_ + ((offset_ + timecode_ms) / 1000);

	do {
		if (timecode_ms == -1) {
			data.update(filter_);

			if (this->retrieveData(data) == GPX::DataEof)
				goto eof;

			result = GPX::DataMeasured;
		}
		else if (data.time(GPXData::PositionCurrent) == timestamp) {
			result = GPX::DataUnchanged;
		}
		else if (timestamp < data.time(GPXData::PositionNext)) {
			data.predict(filter_);

			result = GPX::DataPredicted;
		}
		else if (timestamp == data.time(GPXData::PositionNext)) {
			data.update(filter_);

			if (this->retrieveData(data) == GPX::DataEof)
				goto eof;
	
			result = GPX::DataMeasured;
		}
		else if (timestamp > data.time(GPXData::PositionNext)) {
			if ((data.time(GPXData::PositionCurrent) + 1) >= data.time(GPXData::PositionNext)) {
				data.update(filter_);
			
				if (this->retrieveData(data) == GPX::DataEof)
					goto eof;
			
				result = GPX::DataMeasured;
			}
			else {
				data.predict(filter_);
			
				result = GPX::DataPredicted;
			}
		}
	} while ((timecode_ms != -1) && (data.time(GPXData::PositionCurrent) < timestamp));

	return result;

eof:
	data = GPXData();

	return GPX::DataEof;
}


enum GPX::Data GPX::retrieveData(GPXData &data) {
	gpx::WPT *wpt;
	gpx::TRKSeg *trkseg = (*iter_seg_);

	std::list<gpx::WPT*> &trkpts = trkseg->trkpts().list();
	std::list<gpx::TRKSeg*> &trksegs = trk_->trksegs().list();

	// Next point
	iter_pts_++;

	if (iter_pts_ == trkpts.end()) {
		iter_seg_++;

		if (iter_seg_ == trksegs.end())
			goto done;

		trkseg = (*iter_seg_);

		trkpts = trkseg->trkpts().list();
		iter_pts_ = trkpts.begin();
	}

	if (iter_pts_ == trkpts.end())
		goto done;

	wpt = (*iter_pts_);

	data.read(wpt);

	return GPX::DataMeasured;

done:
	data = GPXData();

	return GPX::DataEof;
}


enum GPX::Data GPX::retrieveLast(GPXData &data) {
	gpx::WPT *wpt;
	std::list<gpx::TRKSeg*> &trksegs = trk_->trksegs().list();

	data = GPXData();

	iter_seg_ = trksegs.end();

	if (!trksegs.empty()) {
		iter_seg_--;
	
		gpx::TRKSeg *trkseg = (*iter_seg_);

		std::list<gpx::WPT*> &trkpts = trkseg->trkpts().list();
		iter_pts_ = trkpts.end();

		if (!trkpts.empty()) {
			iter_pts_--;

			wpt = (*iter_pts_);

			data.read(wpt);

			return GPX::DataMeasured;
		}
	}

	return GPX::DataEof;
}


bool GPX::getBoundingBox(GPXData::point *p1, GPXData::point *p2) {
	GPXData::point p;

	std::list<gpx::TRKSeg*> &trksegs = trk_->trksegs().list();

	p1->valid = false;
	p2->valid = false;

	for (std::list<gpx::TRKSeg*>::iterator iter2 = trksegs.begin(); iter2 != trksegs.end(); ++iter2) {
		gpx::TRKSeg *seg = (*iter2);

		std::list<gpx::WPT*> &trkpts = seg->trkpts().list();

		for (std::list<gpx::WPT*>::iterator iter3 = trkpts.begin(); iter3 != trkpts.end(); ++iter3) {
			gpx::WPT *wpt = (*iter3);

			GPXData::convert(&p, wpt);

			if (!p.valid)
				continue;

			if (!p1->valid)
				*p1 = p;
			if (!p2->valid)
				*p2 = p;

			// top-left bounding box
			if (p.lon < p1->lon)
				p1->lon = p.lon;
			if (p.lat > p1->lat)
				p1->lat = p.lat;

			// bottom-right bounding box
			if (p.lon > p2->lon)
				p2->lon = p.lon;
			if (p.lat < p2->lat)
				p2->lat = p.lat;
		}
	}

	return (p1->valid && p2->valid);
}


double GPX::getMaxSpeed(void) {
	GPXData data;

	double max_speed = 0.0;
	double last_speed = 0.0;

	std::list<gpx::TRKSeg*> &trksegs = trk_->trksegs().list();

	for (std::list<gpx::TRKSeg*>::iterator iter2 = trksegs.begin(); iter2 != trksegs.end(); ++iter2) {
		gpx::TRKSeg *seg = (*iter2);

		std::list<gpx::WPT*> &trkpts = seg->trkpts().list();

		for (std::list<gpx::WPT*>::iterator iter3 = trkpts.begin(); iter3 != trkpts.end(); ++iter3) {
			gpx::WPT *wpt = (*iter3);

			data.read(wpt);

			if (!data.valid())
				continue;

			// Skip incoherent values
			if (abs(last_speed - data.speed()) > 10)
				continue;

			if (data.speed() > max_speed)
				max_speed = data.speed();

			last_speed = data.speed();
		}
	}

	return max_speed;
}

