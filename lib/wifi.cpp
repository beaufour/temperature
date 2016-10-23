// This software is part of OpenMono, see http://developer.openmono.com
// Released under the MIT license, see LICENSE.txt
#include "wifi.hpp"

Wifi::Wifi (IConfiguration const & configuration_)
:
    spi(RP_SPI_MOSI, RP_SPI_MISO, RP_SPI_CLK),
    spiComm(spi, NC, RP_nRESET, RP_INTERRUPT),
    _buffer(0),
    _status(Wifi_NotConnected),
    configuration(configuration_)
{
    printf("Wifi()\r\n");
}

Wifi::~Wifi ()
{
    destroyClient();
}

Wifi::Status Wifi::startConnection ()
{
    printf("Wifi:startConnection()\r\n)");
#ifdef WIFI_SSID
#warning Using static Wifi configuration
    ssid = ByteString(WIFI_SSID);
    password = ByteString(WIFI_PASSWORD);
#else
    if (configuration.get(MONO_WIFI_SSID) == 0 || configuration.get(MONO_WIFI_PASSWORD) == 0)
    {
        printf("No wifi configuration\r\n");
        _status = Wifi_BrokenConfiguration;
        return _status;
    }
    ssid = configuration.get(MONO_WIFI_SSID);
    password = configuration.get(MONO_WIFI_PASSWORD);
#endif

    printf("WIFI - SSID: %s, PW: %s\r\n", ssid.c_str(), password.c_str());

    using mono::redpine::Module;
    Module::initialize(&spiComm);
    Module::setupWifiOnly(ssid.c_str(),password.c_str());
    Module::setNetworkReadyCallback<Wifi>(this,&Wifi::networkReadyHandler);
    _status = Wifi_Connecting;
    return _status;
}

void Wifi::networkReadyHandler ()
{
    printf("Wifi::networkReadyHandler()\r\n");
    _status = Wifi_Ready;
    connectHandler.call();
}

Wifi::Status Wifi::startRequest (uint8_t const * url)
{
    printf("Wifi::startRequest(%s)\r\n", url);

    destroyClient();
    client = mono::network::HttpClient((char const *)url);
    client.setDataReadyCallback<Wifi>(this,&Wifi::httpHandleData);
    client.setErrorCallback<Wifi>(this,&Wifi::errorCallback);
    _status = Wifi_Receiving;
    return _status;
}

void Wifi::errorCallback (mono::network::INetworkRequest::ErrorEvent* error)
{
    printf("Wifi::errorCallback(%i)\r\n", error->ErrorCode);
    statusHandler.call(Wifi_BrokenReply);
    // The assumption is that it is only the current connection that is dead, not the underlying
    // network layer
    _status = Wifi_Ready;
}

void Wifi::httpHandleData (HttpClient::HttpResponseData const & data)
{
    if (data.Context->HasFailed())
    {
        // The assumption is that errorCallback is always called and will take care of the
        // statusHandler call.
        return;
    }

    if (_buffer != 0) {
        _buffer->add((uint8_t const *)data.bodyChunk(),data.bodyChunk.Length());
    }
    if (data.Finished)
    {
        _status = Wifi_Ready;
        statusHandler.call(_status);
        receivedHandler.call(_buffer);
    }
    else
    {
        statusHandler.call(_status);
    }
}

Wifi::Status Wifi::status () const
{
    return _status;
}

void Wifi::destroyClient ()
{
}
