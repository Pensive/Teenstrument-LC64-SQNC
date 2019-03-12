# Teenstrument-LC64-SQNC
An Adafruit Untztrument 64, a Teensy-LC, as a standalone MIDI 32 step sequencer

This project was born from my involvem ent ion the Teeny LC beta testing stage.
I decided getting it working with an Untztrument was a reasonable challenge / test - and 
this little 32 step sequencer has proven that it's a capable little board.

It works great - there's a video of it in operation here https://youtu.be/a-elzPv6dpU

The sequencer really needs to be reprogrammed to use interrupts - I had some challenges using them so I reverted to using metro 
which is not an accurate method of timing.

I never did add all the other instruments but it's a great basis for playing with an untztrument using PJRC Teensy USB midi library.
Free for all - do what you like with it - offered forward with no license or warranty.
