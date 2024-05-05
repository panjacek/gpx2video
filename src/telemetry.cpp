#include <string>
#include <iomanip>
#include <cmath>

#include "log.h"
#include "telemetry.h"


// Telemetry settings
//--------------------

TelemetrySettings::TelemetrySettings() {
}


TelemetrySettings::~TelemetrySettings() {
}


const TelemetrySettings::Filter& TelemetrySettings::filter(void) const {
	return filter_;
}


void TelemetrySettings::setFilter(const TelemetrySettings::Filter &filter) {
	filter_ = filter;
}


const std::string TelemetrySettings::getFriendlyName(const TelemetrySettings::Filter &filter) {
	switch (filter) {
	case FilterNone:
		return "None";
	case FilterSample:
		return "Get sample each second from telemetry data";
	case FilterLinear:
		return "Apply a simple linear filter on telemtry data";
	case FilterInterpolate:
		return "Interpolate telemetry data filter";
	case FilterKalman:
		return "Apply Kalman filter on telemetry data (lat., lon. and ele.)";
	case FilterCount:
	default:
		return "";
	}

	return "";
}



// Telemetry API
//---------------

Telemetry::Telemetry(GPX2Video &app) 
	: Task(app) 
	, app_(app) {
}


Telemetry::~Telemetry() {
}


Telemetry * Telemetry::create(GPX2Video &app) {
	Telemetry *telemetry = new Telemetry(app);

	telemetry->init();

	return telemetry;
}


void Telemetry::init(void) {
	log_call();

	gpx_ = GPX::open(app_.settings().gpxfile(), app_.settings().telemetryFilter());
}


bool Telemetry::start(void) {
	bool result = true;

	std::string filename = app_.settings().outputfile();

	log_call();

	// Telemetry limits
	gpx_->setFrom(app_.settings().gpxFrom());
	gpx_->setTo(app_.settings().gpxTo());

	// Start time activity
	gpx_->retrieveFirst(data_);
//	gpx_->setStartTime(data_.time());

	// Open output stream
    out_ = std::ofstream(filename);
       
	if (!out_.is_open()) {
		log_error("Open '%s' failure", filename.c_str());
		result = false;
		goto done;
	}

	// Header
	out_ << "Timestamp, Time, Total duration, Partial duration, RideTime, Data, Lat, Lon, Ele, Grade, Distance, Speed, MaxSpeed, Average, Ride Average, Cadence, Heartrate, Lap" << std::endl;

	// Read telemetry data from start
//	timecode_ms_ = 0;
	timecode_ms_ = data_.time() * 1000;

done:
	return result;
}


bool Telemetry::run(void) {
	// Read telemetry data each 1 second
	int rate;

	struct tm tm;

	char time[128];

	enum GPX::Data type;

	log_call();

	rate = app_.settings().telemetryRate();

	if (app_.settings().telemetryFilter() == TelemetrySettings::FilterNone) 
		timecode_ms_ = -1;

	if ((type = this->get(data_, timecode_ms_)) == GPX::DataEof) {
		goto done;
	}

	localtime_r(&data_.time(), &tm);
	strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", &tm);

	out_ << std::setprecision(8);
	out_ << data_.time();
	out_ << ", \"" << time << "\"";
	out_ << ", " << round(data_.elapsedTime());
	out_ << ", " << round(data_.duration());
	out_ << ", " << round(data_.rideTime());
	out_ << ", " << data_.type2string();
	out_ << ", " << data_.position().lat;
	out_ << ", " << data_.position().lon;
	out_ << ", " << data_.position().ele;
	out_ << ", " << data_.grade(); 
	out_ << ", " << data_.distance(); 
	out_ << ", " << data_.speed(); 
	out_ << ", " << data_.maxspeed(); 
	out_ << ", " << data_.avgspeed(); 
	out_ << ", " << data_.avgridespeed(); 
	out_ << ", " << data_.cadence(); 
	out_ << ", " << data_.heartrate(); 
	out_ << ", " << data_.lap(); 
	out_ << std::endl;

	if (rate > 0)
		timecode_ms_ += 1000 / rate;
	else
		timecode_ms_ += 1000;

	schedule();

	return true;

done:
	complete();

	return true;
}


bool Telemetry::stop(void) {
	if (out_.is_open())
		out_.close();

	return true;
}


enum GPX::Data Telemetry::get(GPXData &data, int64_t timecode_ms) {
	enum GPX::Data type;

	type = gpx_->retrieveNext(data, timecode_ms);

	return type;
}

