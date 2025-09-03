#include "TrackSense.hpp"


int main(){

	TrackSense ss(
		"127.0.0.1",
		9000,
		"/dev/ttySTM2"
	);

	ss.init();

	std::this_thread::sleep_for(std::chrono::seconds(120));

	ss.stop();

	return 0;
}