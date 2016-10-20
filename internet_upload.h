#ifndef internet_upload_h
#define internet_upload_h

#include <mbed.h>
#include <SDFileSystem.h>
#include <wireless/module_communication.h>
#include <mn_string.h>
#include <http_client.h>

using namespace mono;

class InternetUpload {

    static const char *ssidFilename;
    static const char *passFilename;
    static const char *urlFileName;

    bool fsExist;
    String ssid;
    String password;
    String url;
    timestamp_t lastSendTime;

    SDFileSystem sdfs;
    mbed::SPI rpSpi;
    redpine::ModuleSPICommunication spiComm;
    network::HttpClient client;

    AppController * ctrl;

protected:

    void wifiConnected();

public:

    mbed::FunctionPointer wifiStarted;

    InternetUpload(AppController *);

    void init();

    void connectWifi();

    void beginDownload();

    void httpCompletion(network::INetworkRequest::CompletionEvent *evnt);
};

#endif /* internet_upload_h */
