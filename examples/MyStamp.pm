package MyStamp;
use strict;
use Data::Dumper;
use Jabber::mod_perl qw(:constants);

sub init {

  print STDERR "Inside ".__PACKAGE__."::init() \n";

}

sub xwarn {
  warn "SM  :  XWARN".scalar localtime() ." : ", @_, "\n";
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
  #my $ns = $nad->find_namespace(1,"http://www.w3.org/1999/xhtml","xmlns");
  #my $ns = $nad->find_scoped_namespace("http://www.w3.org/1999/xhtml","xmlns");
  #xwarn "namespace is: $ns";
  my $elhtml = $nad->find_elem(1,-1,"html",1);
  xwarn "BODY XHTML element is: $elhtml";
  my $elx = $nad->find_elem($elhtml,-1,"body",1) if $elhtml;
  my $datax = $nad->nad_cdata( $elx ) if $elx;
  xwarn "Body xhtml element is: $elhtml/$elx - $datax" if $elx;
  #$nad->append_cdata_head($el, "some data or other");
  $nad->replace_cdata_head($el, "beginning... ($data/$el) ...some data or other") if $el > 0;
  $nad->replace_cdata_head($elx, "beginning... ($datax) ...some data or other") if $elx;
  return PASS;
  

  #return PASS unless $chain eq PKT_SM || $chain eq JADPERL_PKT;
#
#  my $nad = $pkt->nad();
#  warn "Packet is: ".$nad->print(1)."\n";
#
#  return PASS unless $type eq MESSAGE;
#
#  return PASS unless $to =~ /^(localhost\/mod_perl|jpcomp)/;
#
#  my $el = $nad->find_elem(1,-1,"body",1);
#  warn "Element is: $el\n";
#  return PASS unless $el > 0;
#
#  $el = $nad->insert_elem($el, -1, "blah", "some data or other");
#
#  my $newpkt = $pkt->dup($from, $to);
#  $pkt->free();
#
#  $newpkt->router();
#
#  return HANDLED;

}

1;
