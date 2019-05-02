# SoundFinder

## Sound Localization on the MSP430

## The Video

<iframe width="560" height="315" src="https://www.youtube.com/embed/_Bp2Un_X9Xs" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

## Overview
For my ELEC 327 ﬁnal project, I designed a sensor board to do sound localization with echo rejection. The board was comprised of 12 SPI LED’s in a circular grid, 4 electret condenser microphones, a battery pack, and an MSP430G2553 embedded microcontroller. This project represents a minimum viable version of the initial concept, and the board features signiﬁcant opportunities for expansion at a later date. 


## The Concept

The idea that motivated the project was the proliferation of networked microphone devices like the Amazon Echo that use only voice to communicate with people. I learned about the concept of beam-forming microphones — using multiple microphones in a known, ﬁxed array to ﬁlter out ambient noise and improve the signal to noise ratio of an audio signal by using directionality — and I became intrigued about the possibilities of using similar technology. The ﬁrst signiﬁcant step that these products perform is to determine where the speaker is relative to the device. They use this information to determine how they manipulate the inputs from each microphone to decode the speech signal. This process is known as “sound localization”, and this project represents my attempt to perform it using the MSP430.


The computations to do sound localization given multiple microphone inputs are relatively simple in theory, but the wave nature of audio require more careful attention and more advanced processing. In general, I discovered that this task is typically considered too computationally intensive in real-time for a 16-bit microcontroller with no hardware audio processing built in. 


There are two major ways to do sound localization, and they are both reﬂected in how people’s brains perform the task. 

### Amplitude Decoding (Inter-aural level difference)


In this method, one reads all microphones simultaneously and determines which one is receiving the sound wave with the largest amplitude. This microphone will be closest to the noise because energy in a sound wave drops as the square of distance from the source, and amplitude is primarily a measure of energy. When we do this in our brains, our ears help us out because of their signiﬁcant directionality (our ears are shaped to allow sound in only from one direction each), and the fact that our heads attenuate the level of incoming sound signiﬁcantly (mostly high frequency sound) so that our ears hear diﬀerent amplitudes often. 


However, on a typical PCB, the microphones are very close together and their directionality is not always ideal (I could not ﬁnd highly directional microphones for this project). This makes this approach more diﬃcult.


### Phase Delay Decoding (Inter-aural time difference)

Phase delay decoding involves measuring the time diﬀerence between when each microphone measure the same sound wave. This is probably the most accurate method, but it requires signiﬁcant processing of real-time sound data. This would allow you to determine the phase delay of the sound wave, and the one with the least delay is the nearest to the source. This method is superior for low frequency sound since the phase diﬀerence can be more easily determined since it has a larger period. The way I thought to do this is to sample each channel as quickly as possible, store the values in an array, then at ﬁxed intervals perform a DFT on each channel to determine the frequencies in the sound. Then it would be easy to determine which microphone observed which frequency ﬁrst (since frequency does not attenuate or change over distance we can be sure all speakers will hear each frequency). This probably isn’t feasible in real time on the MSP430. 


## The Final Product 


The SoundFinder board is able to reliably localize sound from short distances to a precision of 360 / 4 = 90 degrees, mostly relying on the amplitude diﬀerence method. Performance is better on high frequency than low frequency sound.



 It presents the direction of incoming sound in a visually pleasing and intuitive way using a ring of LEDs, by lighting up the three LEDs in the circle closest to the source of the sound. 


The graphical display has persistence, meaning that the LEDs stay lit until the next sound it processes. 


It is battery powered, and I project a long battery life given the attempts I made to put the system into low power state between samples. 



## Technical Details

The microphones are connected to analog input channels A0 - A3, which are sampled using the “sequence of channels” mode on the MSP430’s ADC10 module. In this mode, the highest channel is speciﬁed and the ADC samples and converts that channel, and then each subsequent lower channel, until it reaches A0. It then waits. 


Rather than read each channel directly from ADC10MEM register, I use the Data Transfer Controller to automatically move the data to an array in memory without the intervention of the CPU. This setup can achieve the relatively high sampling rates that the MSP is capable of at 10 bit precision. 


However, I did slow this sampling process down by choice because a race condition between the CPU and the DTC presented itself, whereby the CPU would read the array before it was fully populated. To ﬁx this, (rather than polling because that’s bad on power management) and to ensure a more consistent sampling period, I wrote the device into Low Power Mode 0 (CPU oﬀ) as soon as the ADC began sampling the ﬁrst channel. This switches oﬀ the CPU and prevents the data race while the DTC ﬁnished. I set up the ADC10 interrupt service routine handler (which runs every time the ADC10 ﬁnishes the last channel in the sequence and the DTC has completed transfer) to reset the LPM0 bits when it runs. I had some trouble with getting stuck in LPM0 occasionally, so I also used the Watchdog Timer to trigger an ISR every 250ms that would write out of LPM0. I calculated this period to be signiﬁcantly larger than the period the ADC and DTC would need, so it would not interfere by exiting LPM too early and defeating the purpose. 


The ADC10CLK was conﬁgured to be SMCLK, sourced from the DCO at 1MHz. MCLK was also sourced from the DCO, but at 8MHz. 


The logic itself involved several comparisons between the four input channels, taking into account a given threshold diﬀerence value to adjust for sensitivity. I wrote several helper functions to ensure readable code, including one that returns the type-safe absolute value of the diﬀerence of two unsigned integers, and several helper functions to modify the LED array in place. 


First I compared the diagonal and anti-diagonal volume diﬀerences, chose the highest, then compared that against the two microphones to either side of it. I assigned the LEDs based on the results of these conversions. This approach leads to a lot of ﬂexibility and upgradeability as compared to ﬁnding the max of the array, since you can change or add if statements for granular control of real world performance. 


Finally, the LED’s are controlled via SPI by the USCI Module B. 

## Possible Future Improvements


As I mentioned earlier, both the project idea and the actual PCB have signiﬁcant potential for future expansion. 


Here are some things that could be done with little to no hardware modiﬁcation:


1. It would be trivial given the setup I already have to improve the precision of the system by making it truly continuous. However, in my limited testing this proved much more error prone, so I dropped it.


2. Determining the loudness threshold via an average-diﬀerence could give a more consistent reading in an environment with background noise. 


3. (related to number 2) The color of the LEDs could be changed to give an indication of volume relative to average ambient noise.


4. A boxcar ﬁlter could be placed on the readings to reduce noise. 


5. More directional microphones could be substituted, increasing the range on which the system might work.
 6. The other method I mentioned, Phase Delay Decoding, may be implemented for better handling of low frequency sound.


In this project, I rely on a small averaging circuit embedded in the microphone board to determine the amplitude of the sound in hardware so that I don’t have to do that in software. To aid in the development of methods that require directly sampling the audio, such as the Phase Delay method, I have included a jumper that can be set on each channel for direct audio input. 


[Link](url) and ![Image](src)
```


