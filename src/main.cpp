#include "GPSTrack.hpp"

int main(){

	GPSTrack ss(
		"127.0.0.1",
		9000,
		"/dev/ttySTM2"
	);

	ss.init();

	std::this_thread::sleep_for(std::chrono::seconds(60));

	ss.stop();

	return 0;
}