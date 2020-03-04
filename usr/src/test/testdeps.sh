killall s16.registryd  || true
rm /var/tmp/repository || true
app/registryd/s16.registryd &
usleep 10000
app/svccfg/svccfg import ../../test/a.ucl
app/svccfg/svccfg import ../../test/b.ucl
app/svccfg/svccfg import ../../test/c.ucl
usleep 10000
app/restartd/s16.restartd
usleep 10000

killall s16.registryd  || true
