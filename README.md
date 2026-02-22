# Snes-cart-reader
It is a arduino-based super nintendo cart reader. Can read rom and ram of most of the snes carts.

This project is based on this one: https://github.com/cthill/SNESDump and I have used too information from this one https://github.com/sanni/cartreader.

This project has a dual purpose: on one hand, it's a cartridge reader that can dump the ROM and SRAM of a Super Nintendo cartridge, and on the other, when used with an emulator, it can be used to create a retro console that reads Super Nintendo cartridges, relying on an emulator.

In this Git repository, I'll focus on the cartridge reading aspect, and I'll create another one for the complete project: reader + emulator = retro console.


<img src="https://github.com/fisicomolon-git/Snes-cart-reader/blob/main/images/cart%20reader%20front.jpg" >

The code for this project has two parts. First, there's the Arduino Nano firmware, which reads the cartridge's ROM and SRAM and can also write to the SRAM. Second, there's the Python code that receives and stores the data sent by the Arduino.
