### i-Miev
Tests has been made on old i-Miev's but without success: https://github.com/osexpert/ccs32clara-chademo/issues/7
From logs collected, the car ask for amps and the charger gradually increase voltage up to max 370v, but no amps ever flow.
Meaning, CAN and signalling works, but seem like there is no high voltage contact. I suspected the contactors did not close and added more delay to give more time for contactors to close, but no success.
I remebered when probing the GPIO's and were testing chademo d1 and d2. I heard a click when activating d1, but nothing when activating d2. Why was d2 not using a relay? In the chademo circuit schema, d1 and d2 are two equal switches and the chademo spec says charger must be able to continously deliver 12V 2A between pin2(d1) and pin10(d2).
So I assumed d2 was a semiconductor. It could still be a solid state relay, but seemed unlikely when a cheaper mechanical relay would have sufficed.

To make sure I tested with a 12V 10W and 12V 21W light bulb:

Between d1 and d2:
- Unloaded: 13.8V
- 10w load: 11v 0.64A = 7W
- 21w load: 9.4v 1.3A = 12.2W

Between d1 and gnd:
- Unloaded: 14.3V
- 10w load: 12.5v 0.68A = 8.5W
- 21w load: 14V 1.7A = 23W

Suspicions confirmed: between d1 and d2 the adapter can only deliver half (at best) of what the spec demands.
Maybe the makers believed 12V 2A was only between d1 and gnd and not between d1 and d2. The spec/circuit schema shows it clearly, but maybe they never saw it and use trial end error and found it worked, because it _does_ work on newer cars:-)
So this is two wrongs make a right: the adapter does not follow the spec/circuit schema, but newer chademo cars also does not:-D So then we have the unlucky i-Miev's that actually followed the spec, but do not work.
Note: 12V 2A demand was introduced in chademo 1.0 spec. i-Miev predate chademo 1.0, for these cars the amps between d1 and d2 were undefined. But I think it is unlikely that i-Miev draw more than 2A, it would be odd to define 2A in the spec if they knew early i-Mievs would draw more than this.

Similar finding by people rebuilding cars to support chademo, that even some genuine chademo chargers deliver too little amps between d1 and d2 to power a car that follow the spec: https://openinverter.org/forum/viewtopic.php?p=50262#p50262 Their solution: add a relay or use staggered contactors (the latter will involve way to much work in this case, I assume both contactors must be replaced and completely rewired).

To understand the following, need to know how d1, d2, gnd related to which pins in the chademo plug:
- pcb(d1) -> pin2
- pcb(d2) -> pin10 (need to cut)
- pcb(gnd) -> pin1

What can be done?
It is possibly to add a relay inside the adapter, eg. HF49FD-012-1H11T from aliexpress. Coil:12V Switching:5A 30VDC. T=AgSnO2 (Silver Tin Oxide, should be better for inductive loads than the one without T). H11 vs H12 is pin spacing and only matter if pcb mounted.
The wire going from pcb(d2) to pin10 need to be cut. The relay coil(+) would be powered by constant 12v from pcb (or hook into wire going to pin2). The coil(-) should be powered from the cut wire going from pcb(d2).
One of the relay switching pins should go to gnd on the pcb (or hook into wire going to pin1) and the other switching pin connects to cut wire going to pin10.
The problem is, it is very tight inside the adapter, and especially at the end of the chademo plug. It can be done, but not easily.

The alternative is to add a relay inside the car instead. Someone added a button to i-Miev to control the contactors: https://300mpg.org/2021/11/11/chademo-relay-hack-switch-and-testing/ and video https://www.youtube.com/watch?v=hBV9CIVN0fs. The prodecure will be similar, but instead of adding a button, add a relay that power the contactors by using the car 12v battery. d1 and d2 will now only power the added relay.
The bonus with modifying the car is, that the car also work on stubborn chademo chargers that does not follow the spec, as mentioned above (similar findings).
WARNING: it does not _seem_ like the procedure exposes high voltage wires, but be carefull.

I don't own a i-Miev, so I can not test it. I could modify my adapter, but it would only prove it is possible to add a relay, but this I already know:-)

Update: I ended up modifying my own adapter, so you don't have to:-) [Add D2 Relay](add_d2_relay/readme.md)

WARNING: there is no guarantee that adding a relay will make it work, you may create new problems and there may be additional problems as well.

Discussion here: https://github.com/osexpert/ccs32clara-chademo/discussions/34

### Orion BMS
Some has tried the adapter on Orion BMS without luck: https://github.com/osexpert/ccs32clara-chademo/discussions/50
The log show similar behaviour as iMiev: After D2 is set, car start asking for amps, but no amp flowing.
Charger start increasing the voltage, eventually reaching max voltage. But no amps flowing. Charger eventually fails.
It is the same behaviour as iMiev: HV seems to be open circuit (car contactors not closing).
Looking at the Orion BMS manual, I see the problem: https://www.orionbms.com/manuals/pdf/chademo_integration.pdf
They are following the suggested setup from the Chademo spec. and seem to be powering the car contactors directly from D2.
So either it has to be rewired with a setup that a modern Chademo car uses (they do not power contactors directly from D2),
or maybe try a different brand adapter eg. Electway (small chance that other adapter brands follow the Chademo spec better).