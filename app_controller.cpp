// This software is part of OpenMono, see http://developer.openmono.com
// Released under the MIT license, see LICENSE.txt
#include "app_controller.h"

#include "bitmaps/mono_bitmap.h"
#include "bitmaps/temperature_menlo_bitmap.h"
#include "bitmaps/wifi_logo.h"

using mono::sensor::ITemperature;
using mono::display::Color;
using mono::TouchEvent;
using mono::geo::Point;
using mono::display::IDisplayController;
using mono::IApplicationContext;
using mono::String;

#define MONO_TEMPERATURE_URL (uint8_t const *)(CONF_KEY_ROOT "/temperature/url.txt")


Toucher::Toucher (AppController * ctrl_)
:
	ctrl(ctrl_)
{
}

void Toucher::RespondTouchEnd(TouchEvent &)
{
	ctrl->handleTouchEvent();
}

AppController::AppController ()
:
	sleeper(10*60*1000,true),
	toucher(this),
	graphView(40,220-87),
	bg(mono::display::BlackColor),
	timer(1000),
	useCelcius(false),
        lastTemp(0),
        wifi(NULL)
{
	sleeper.setCallback(mono::IApplicationContext::EnterSleepMode);
    displayWifiLogo = false;
}

void AppController::monoWakeFromReset ()
{
	bg.show();
	timer.setCallback<AppController>(this,&AppController::measureAndUpdate);
	timer.Start();
#ifdef WIFI_URL
        #warning Using static Wifi URL configuration
        url = String(WIFI_URL);
#else
        url = configuration.get(MONO_TEMPERATURE_URL);
#endif

        initWifi();

	sleeper.Start();
}

void AppController::monoWakeFromSleep ()
{
    IDisplayController* ctrl = IApplicationContext::Instance->DisplayController;
    ctrl->setBrightness(255);

    // Reset temp to 0, or we'll only slowly flatten up/down from the last temp, which can be much
    // lower or higher.
    lastTemp = 0;

    initWifi();
}

void AppController::monoWillGotoSleep ()
{
    displayWifiLogo = false;
}

int AppController::readTemperatureInCelcius ()
{
#if TEST_TEMP
	static float tempC = 0.0;
	static bool up = true;
	if (up) tempC += 0.5;
	else tempC -= 0.5;
	if (tempC > 50.0) up = false;
	if (tempC < 0.0) up = true;
	lastTemp = tempC * 1000;
#else
	const float flatten = 0.5;
	ITemperature * temperature = IApplicationContext::Instance->Temperature;
        if (lastTemp) {
            lastTemp = (lastTemp * flatten) + (temperature->ReadMilliCelcius() * (1.0 - flatten));
        } else {
            lastTemp = temperature->ReadMilliCelcius();
        }
#endif
	return lastTemp;
}

void AppController::handleTouchEvent ()
{
    static bool display_on = true;
    IDisplayController* ctrl = IApplicationContext::Instance->DisplayController;
    ctrl->setBrightness(display_on ? 0 : 255);
    display_on = !display_on;

    // TODO: this kills networking
    // sleeper.Start();
	useCelcius = ! useCelcius;
	update();
}

int AppController::getLastTemperatureInCelcius ()
{
    return lastTemp;
}

String formatTemperature (int whole, int decimals, char unit)
{
	char string[20];
	if (decimals < 10) sprintf(string,"%i.%i0o%c",whole,decimals,unit);
	else sprintf(string,"%i.%io%c",whole,decimals,unit);
	return string;
}

void AppController::update ()
{
	showLogo();

    if (displayWifiLogo)
    {
        blitImage
        (
         Point(5,8),
         (uint8_t*)mono::display::wifi_logo,
         mono::display::wifi_logo_width,
         mono::display::wifi_logo_height,
         mono::display::CloudsColor
         );
    }

	String s;
	float tempC = getLastTemperatureInCelcius() / 1000.0;
	if (useCelcius)
	{
		int wholeC = tempC;
		int decimalsC = (tempC - wholeC) * 100.0;
		if (decimalsC < 0.0) decimalsC = -decimalsC;
		s = formatTemperature(wholeC,decimalsC,'C');
	}
	else
	{
		float tempF = tempC * 9.0 / 5.0 + 32.0;
		int wholeF = tempF;
		int decimalsF = (tempF - wholeF) * 100.0;
		if (decimalsF < 0.0) decimalsF = -decimalsF;
		s = formatTemperature(wholeF,decimalsF,'F');
	}
	uint8_t x = 176-s.Length()*22;
	drawTemperature(s,x,220-45);
	blitColor(Point(0,220-45),x,45,mono::display::BlackColor);
}

void AppController::measureAndUpdate ()
{
	float tempC = readTemperatureInCelcius() / 1000.0;
	graphView.setNextPoint(tempC);
	graphView.show();
	update();
        uploadData();
}

void AppController::uploadData () {
    if (wifi->status() != Wifi::Wifi_Ready) {
        printf("Wifi not ready: %i\r\n", wifi->status());
        return;
    }

    String full_url = String::Format("%s?celcius=%i", url(), getLastTemperatureInCelcius());
    wifi->get((uint8_t *) full_url(), this);
}

uint8_t translateCharToFontIndex (char ch)
{
	// 0123456789.-oCF = 15
	if (ch >= '0' && ch <= '9') return ch-'0';
	else switch (ch)
	{
		case '.': return 10;
		case '-': return 11;
		case 'o': return 12;
		case 'C': return 13;
		case 'F': return 14;
	}
	return 0;
}

void AppController::drawTemperature (String temperature, uint8_t x, uint8_t y)
{
	int const width = 22;
	for (size_t i = 0; i < temperature.Length(); ++i)
	{
		blitChar(translateCharToFontIndex(temperature[i]),x+i*width,y);
	}
}

void AppController::initWifi ()
{
    if (wifi != NULL) {
        delete wifi;
    }
    wifi = new Wifi(configuration);
    wifi->connect<AppController>(this,&AppController::networkReadyHandler);
}

void AppController::networkReadyHandler ()
{
    printf("AppController::networkReadyHandler()\r\n");
    displayWifiLogo = true;
}

void AppController::blitChar (int index, uint8_t x, uint8_t y)
{
	// 0123456789.-oCF = 15
	int const width = 22;
	int const height = 42;
	int offset = 1 + index*width;
	if (index >= 2)
	{
		offset -= 1;
	}
	if (index >= 7)
	{
		offset -= 1;
	}
	if (index >= 8)
	{
		offset -= 1;
	}
	if (index >= 10)
	{
		offset -= 1;
	}
	if (index >= 14)
	{
		offset -= 1;
	}
	for (int line = 0; line < height; ++line)
	{
		blitImage
		(
			Point(x,y+line),
			(uint8_t*)mono::display::temp_bitmap + (line*326) + offset,
			width,
			1,
			mono::display::CloudsColor
		);
	}
}

void AppController::showLogo ()
{
	blitImage
	(
		Point((176-mono::display::mono_bitmap_width)/2,0),
		(uint8_t*)mono::display::mono_bitmap,
		mono::display::mono_bitmap_width,
		mono::display::mono_bitmap_height,
		mono::display::TurquoiseColor
	);
}

void AppController::blitImage(Point const &p, uint8_t *data, int w, int h, mono::display::Color color, uint8_t preScale)
{
	IDisplayController *ctrl = IApplicationContext::Instance->DisplayController;
	ctrl->setWindow(p.X(), p.Y(), w, h);
	int len = w*h;
	for (int b=0; b<len; b++) {
		ctrl->write(color.scale(data[b]).scale(preScale));
	}
}

void AppController::blitColor(Point const &p, int w, int h, mono::display::Color color, uint8_t preScale)
{
	IDisplayController *ctrl = IApplicationContext::Instance->DisplayController;
	ctrl->setWindow(p.X(), p.Y(), w, h);
	int len = w*h;
	for (int b=0; b<len; b++) {
		ctrl->write(color.scale(preScale));
	}
}
