## How to flash release

1.  Download and install [Ti's Uniflash tool](http://www.ti.com/tool/uniflash).
2.  Open deploy/uniflash_config.usf file from Uniflash application.
3.  Do not forget to place SOP2 jumper on the cc3200 Launchpad so it is in flash mode.
4.  (optional) Under CC3100/CC3200 Config Groups you can configure role (AP/station) and you WiFi profile. Also, you can use [Smart Toybox ThemePacker](https://github.com/nsumrak/Smart-Toybox-ThemePacker) to customize theme.stb found in deploy/fileSystem directory.
5.  Enter virtual COM port (you can find it via Device Manager) of Launchpad device and click "Program".
6.  After flashing disconnect device and remove SOP2 jumper.
7.  Enjoy.
