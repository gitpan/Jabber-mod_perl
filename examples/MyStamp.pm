package MyStamp;
use strict;
use Data::Dumper;
use Jabber::mod_perl qw(:constants);

sub init {

  print STDERR "Inside ".__PACKAGE__."::init() \n";

}

sub xwarn {
  warn "SM  : ".scalar localtime() ." : ", @_, "\n";
}

sub handler {
  
  my $class = shift; 
  my ($pkt, $chain, $instance) = @_; 
  xwarn "Inside ".__PACKAGE__."::handler()";
  xwarn "Packet is: ".$pkt->nad()->print(1);
  xwarn "chain is: $chain instance is: $instance";
  my $to = $pkt->to();
  my $from = $pkt->from();
  my $type = $pkt->type();
  xwarn "The to address is: ".$to;
  xwarn "The from address is: ".$from;
  xwarn "The type is: ".$type;
  return PASS unless $type eq "message";;
  my $nad = $pkt->nad();
  my $el = $nad->find_elem(1,-1,"body",1);
  my $data = $nad->nad_cdata( $el );
  xwarn "Body element is: $el - $data";
  $nad->append_cdata_head($el, "some data or other");
  return PASS;
  
}

1;
