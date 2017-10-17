Smart Toybox
============

Smart Toybox is a hardware device that solves problem most parents have - toys all over the house by making cleaning-up the toys FUN!

![Kids will love to clean-up their toys!](docs/smart-toybox.jpg)

This repository contains Smart Toybox embedded software for Smart Toybox prototype based on TI cc3200 platform.

Requirements
------------
This software require previously built hardware.

How to build it
---------------
We recommend using TI CCS edition that is freely available for cc3200 platform. Installation manual is available with cc3200 evaluation board used as platform
for hardware and it is assumed that you already installed and configured CCS
for cc3200. You will also need TI's Uniflash tool.

1.  Create new CCS empty project named smarttoybox. Select target cc3200.
2.  Clone repo into created project folder.
    > Searching for better solution because this way whenever important change is made in project
    > it needs to be propagated to project files in ccs directory.
    > Also when change comes via source control, ccs dir content needs to be copied back to project root dir.
3.  Compile the project.
4.  (optional) customize theme using Theme Packer. You can find theme.stb file in
    deploy/uniflash_config_session/fileSystem directory.
5.  Use TI's Uniflash tool to setup device using uniflash_config.usf file in
    deploy directory.
6.  On the first run, you will have to calibrate your weight sensor.
    Prepare 1.5l bottle of water or similar weight. Run the project and follow console output. When instructed put bottle (weight) on the plate atteched
    to the sensor.
7.  Edit settings.ini file in deploy/uniflash_config_session/fileSystem directory
    with data gathered in calibration so you can flash the board with calibrated data and ommit calibration. If you want to rerun calibration just delete settings.ini or set all the valuse to 0.
8.  After testing your device you can remove SOP jumper from cc3200 board,
    connect power bank and have FUN!

Additional Open Source code included
------------------------------------

We are using (slightly modified to compile and work on two channels) WavPack integer-based decompression code called Tiny Decoder, hardware-friendly decoder
by David Bryant.

You can find original code [here: http://wavpack.com/](http://wavpack.com/)
