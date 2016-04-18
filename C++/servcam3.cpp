#include <iostream>
#include <evhttp.h>
#include <memory>
#include <cstdint>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
using namespace std;
using namespace cv;

VideoCapture cap(0); // Initialize the camera. This has to be global so the callback function recognizes it.

int main()
{
	/*----- Starting camera -----*/
	
	if(!cap.isOpened())
	{
		cerr << "Cannot open the camera." << endl;
		return -1;
	}
	
	/*----- Starting server -----*/
	
	if(!event_init())
	{
		cerr << "Failed to init libevent." << endl;
		return -1;
	}
	
	char const SrvAddress[] = "0.0.0.0"; // Server IP (should be same as device IP; 0.0.0.0 binds to all available wireless and wired addresses)
	uint16_t SrvPort = 80; // Port number (FRC requires a specific port for camera servers)
	unique_ptr<evhttp, decltype(&evhttp_free)> Server(evhttp_start(SrvAddress, SrvPort), &evhttp_free); //Start server
	
	if(!Server)
	{
		cerr << "Failed to init http server." << endl;
		return -1;
	}
	
	/*----- Callback to display image from camera -----*/
	
	void (*OnReq)(evhttp_request *req, void *) = [] (evhttp_request *req, void *)
	{
		auto *OutBuf = evhttp_request_get_output_buffer(req);
		if(!OutBuf)
			return;
	
		Mat frame; // Make an image variable for the current frame
		
		bool Success = cap.read(frame); // Read the current frame from the camera and check for success
		if(!Success)
		{
			cerr << "Cannot read frame from camera." << endl;
			return;
		}
		
		cout << "Size of\n"
			 << "frame ...... " << sizeof(frame) << " bytes\n"
			 << "height x width: " << frame.cols << " x " << frame.rows << ".\n";
		
		vector<uchar> buf;
		imencode(".jpg", frame, buf); // Encode Mat frame to jpeg format
		char *enc_msg = new char[buf.size()];
		for(int i=0; i < buf.size(); i++) enc_msg[i] = buf[i];
		
		evhttp_add_header(evhttp_request_get_output_headers(req),"Content-type", "image/jpg");
		
		evbuffer_add(OutBuf, enc_msg, buf.size()); // Add const char * to OutBuf
		
		cout << "frame2 ..... " << sizeof(frame) << " bytes\n"
			 << "buf ........ " << sizeof(buf) << " bytes\n"
			 << "buf ........ " << buf.size() << " bytes\n"
			 << "enc_msg .... " << sizeof(enc_msg) << " bytes\n"
			 << "*enc_msg ... " << sizeof(*enc_msg) << " bytes\n"
			 << "&enc_msg ... " << sizeof(&enc_msg) << " bytes\n"
			 << "OutBuf ..... " << sizeof(OutBuf) << " bytes\n";
			 
		evhttp_send_reply(req, HTTP_OK, "", OutBuf); // Reply to HTTP request with the contents of OutBuf. This will empty but not release OutBuf.
		
		cout << "Outbuf2 .... " << sizeof(OutBuf) << " bytes\n";
		cout << "Request complete\n===============\n\n";
	};
	evhttp_set_gencb(Server.get(), OnReq, nullptr);
	if(event_dispatch() == -1)
	{
		cerr << "Failed to show frame." << endl;
		return -1;
	}
	return 0;
}