#include "NATPunchHandler.h"
#include "../raknet/TCPInterface.h"
#include "../raknet/HTTPConnection.h"
#include "PHPDirectoryServer2.h"

using namespace RakNet;
NATPuchHandler::NATPuchHandler() {
	tcpInterface = new TCPInterface;
}
NATPuchHandler::~NATPuchHandler() {
	delete tcpInterface;
}

void NATPuchHandler::initialize() {
	tcpInterface->Start(0, 64);
}

void NATPuchHandler::registerToGameList(const RakNet::RakString& serverName, int port) {
	HTTPConnection httpConnection;
	httpConnection.Init(tcpInterface, "johanbernhardsson.se");
	PHPDirectoryServer2 directoryServer;
	directoryServer.Init(&httpConnection, "/DirectoryServer.php");
	directoryServer.UploadTable("", serverName, port, true);
}

void NATPuchHandler::removeFromGameList() {

}

void NATPuchHandler::close() {

}

