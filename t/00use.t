# Need to suppress warinings ?
BEGIN { $^W = 0; $| = 1; print "1..1\n"; }
END {print "not ok 1\n" unless $loaded;}
use Jabber::mod_perl qw(:constants);
$loaded = 1;
print "ok 1\n";
