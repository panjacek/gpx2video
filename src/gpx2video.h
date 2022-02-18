#ifndef __GPX2VIDEO__GPX2VIDEO_H__
#define __GPX2VIDEO__GPX2VIDEO_H__

#include <iostream>
#include <cstdlib>
#include <string>
#include <list>

#include <unistd.h>

#include "log.h"
#include "media.h"
#include "mapsettings.h"
#include "extractorsettings.h"


class Map;
class Extractor;

class GPX2Video {
public:
	class Settings {
	public:
		Settings(std::string gpx_file="", 
			std::string media_file="", 
			std::string layout_file="",
			std::string output_file="",
			int offset=0,
			double map_factor=1.0,
			int map_zoom=8, 
			int max_duration_ms=0,
			MapSettings::Source map_source=MapSettings::SourceOpenStreetMap,
			ExtractorSettings::Format extract_format=ExtractorSettings::FormatDump)
			: gpx_file_(gpx_file)
			, media_file_(media_file)
			, layout_file_(layout_file)
			, output_file_(output_file)
			, offset_(offset)
			, map_factor_(map_factor)
			, map_zoom_(map_zoom)
			, max_duration_ms_(max_duration_ms)
			, map_source_(map_source)
	   		, extract_format_(extract_format) {
		}

		const std::string& gpxfile(void) const {
			return gpx_file_;
		}
		
		const std::string& mediafile(void) const {
			return media_file_;
		}

		const std::string& layoutfile(void) const {
			return layout_file_;
		}

		const std::string& outputfile(void) const {
			return output_file_;
		}

		const int& offset(void) const {
			return offset_;
		}

		const MapSettings::Source& mapsource(void) const {
			return map_source_;
		}

		const ExtractorSettings::Format& extractFormat(void) const {
			return extract_format_;
		}

		const double& mapfactor(void) const {
			return map_factor_;
		}

		const int& mapzoom(void) const {
			return map_zoom_;
		}

		const int& maxDuration(void) const {
			return max_duration_ms_;
		}

	private:
		std::string gpx_file_;
		std::string media_file_;
		std::string layout_file_;
		std::string output_file_;

		int offset_;

		double map_factor_;
		int map_zoom_;
		int max_duration_ms_;
		MapSettings::Source map_source_;

		ExtractorSettings::Format extract_format_;
	};

	class Task {
	public:
		typedef void (*callback_t)(void *object);

		Task(GPX2Video &app)
			: app_(app) {
		}

		virtual ~Task() {
		}

		virtual void run(void) = 0;

		void schedule(void) {
			app_.perform();
		}

		void complete(void) {
			app_.perform(true);
		}

	private:
		GPX2Video &app_;
	};

	enum Command {
		CommandNull,

		CommandSource,	// Dump map source list
		CommandFormat,  // Dump extract format supported
		CommandSync,	// Auto sync video time with gps sensor
		CommandExtract,	// Extract gps sensor data from video
		CommandClear,	// Clear cache directories
		CommandMap,		// Download & build map
		CommandTrack,	// Download, build map & draw track
		CommandVideo,	// Render video with telemtry overlay

		CommandCount
	};

	GPX2Video(struct event_base *evbase);
	~GPX2Video();

	void setLogLevel(int level);
	static const std::string version(void);

	Settings& settings(void);
	void setSettings(const Settings &settings);

	Command& command(void);
	void setCommand(const Command &command);

	int parseCommandLine(int argc, char *argv[]);

	void append(Task *task) {
		tasks_.push_back(task);
	}

	struct event_base *evbase(void) {
		return evbase_;
	}

	MediaContainer * media(void);
	Map * buildMap(void);
	Extractor * buildExtractor(void);

	const time_t& time(void) {
		return time_;
	}

	void setTime(const time_t &time) {
		time_ = time;
	}

	void perform(bool done=false) {
		int32_t info;

		log_call();

		info = (int32_t) done;

		write(pipe_out_, &info, sizeof(info));
	}

	void run(bool done) {
		if (done && !tasks_.empty())
			tasks_.pop_front();

		if (!tasks_.empty()) {
			Task *task = tasks_.front();

			task->run();
		}
		else
			abort();
	}

	void exec(void) {
		log_call();

		perform();
		loop();
	}

	void abort(void) {
		log_call();

		loopexit();
	}

protected:
	static void sighandler(int sfd, short kind, void *data);
	static void pipehandler(int sfd, short kind, void *data);

	void init(void);

	void loop(void);
	void loopexit(void);

private:
	int pipe_in_;
	int pipe_out_;

	struct event *ev_pipe_;
	struct event *ev_signal_;
	struct event_base *evbase_;

	Command command_;
	Settings settings_;

	MediaContainer *container_;

	std::list<Task *> tasks_;

	time_t time_;
};

#endif
