# WeatherStation
My Weather Station daemon and perl web cgi. Radio Shack Wx-200 and Lacrosse
2300 support. This code has run continuously since around 1997. The Radio Shack
WX-200 lasted most of those years ... it was a great value. The LaCrosse is
crap... the rain guage is crap and the wireless is crap.

Honestly, I'm switching to a Davis VantageVue and WeeWx even though I really
don't like Python. This code will hopefully be abandoned shortly.

There are two main branches in the code: 'master' and 'LaCrosseWx'. The
LaCrosseWx branch is what's running now and gets maintenance. It depends on
Open2300.

The logger is a pthreads C program that waits for the weather station to blurt
out a reading. It looks at it and logs it to a Postgresql database if there are
any changes. It's called 'wxd'.

