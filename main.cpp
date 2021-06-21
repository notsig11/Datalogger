#include<iostream>
#include<string>
#include<cstring>

#include<net/if.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<sys/socket.h>

#include<linux/can.h>
#include<linux/can/raw.h>

#include<asio.hpp>
#include<boost/bind.hpp>

void print_frame(struct can_frame frame) {
	std::cout << "Received frame (DL " << static_cast<int>(frame.can_dlc) << "): " << 
			std::hex << frame.can_id << "\t";

	for (int i=0; i<frame.can_dlc; ++i) {
		std::cout << std::hex << static_cast<int>(frame.data[i]) << " ";
	}
	std::cout << "\n";
}

void frame_handler(struct can_frame frame, asio::posix::stream_descriptor& stream) {
	print_frame(frame);
	stream.async_read_some(asio::buffer(&frame, sizeof(frame)), std::bind(frame_handler, std::ref(frame), std::ref(stream)));
}

int main(int argc, char* argv[]) {
	int sock;
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame f;

	if (argc < 2) {
		std::cerr << "Must specify the CAN interface name ex: can0\n";
		return -1;
	}

	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);
	std::cout << "iface: " << ifr.ifr_name << "\n";

	if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket create error...");
		return -1;
	}

	ioctl(sock, SIOGIFINDEX, &ifr);

	std::memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind error...");
		return -1;
	}

	asio::io_service ios;
	asio::posix::stream_descriptor stream(ios);
	stream.assign(sock);

	stream.async_read_some(asio::buffer(&f, sizeof(f)), std::bind(frame_handler, std::ref(f), std::ref(stream)));

	ios.run();
}
