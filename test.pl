#!/usr/bin/perl
use ExtUtils::testlib;
use Wiimote;

print qq!
         test.pl - libcwiimote perl module test application

         A    - hold to Enable accelerometer
         1    - hold to Enable rumble
         Home - Exit

        Press buttons 1 and 2 on the wiimote now to connect.
!;

my $wii = new Wiimote;

print "Connect " . $wii->wiimote_connect('00:19:1D:75:CC:30');
print "\n-------------------\n";

print "Is open: " . $wii->is_open();
print "\n-------------------\n";
while ( $wii->is_open() ) {
    $wii->wiimote_update();
    $wii->set_wiimote_rumble(0);
    if ( $wii->get_wiimote_keys_1 ) {
        $wii->set_wiimote_rumble(1);
    }
    else {
        $wii->set_wiimote_rumble(0);
    }
    if ( $wii->get_wiimote_keys_a ) {
        $wii->activate_wiimote_accelerometer();
    }
    else {
        $wii->deactivate_wiimote_accelerometer();
    }

    print "Wiimote Key bits: " . $wii->get_wiimote_keys_bits();
    print "\n";

    print "Wiimote X axis : " . $wii->get_wiimote_axis_x();
    print " y axis : " . $wii->get_wiimote_axis_y();
    print " z axis : " . $wii->get_wiimote_axis_z();
    print "\n";

    print "Wiimote X tilt : " . $wii->get_wiimote_tilt_x();
    print " y tilt : " . $wii->get_wiimote_tilt_y();
    print " z tilt : " . $wii->get_wiimote_tilt_z();
    print "\n";

    print "nunchuck X : " . $wii->get_wiimote_ext_nunchuk_axis_x();
    print " Y : " . $wii->get_wiimote_ext_nunchuk_axis_y();
    print " Z : " . $wii->get_wiimote_ext_nunchuk_axis_z();
    print " Key c : " . $wii->get_wiimote_ext_nunchuk_keys_c();
    print " Key z : " . $wii->get_wiimote_ext_nunchuk_keys_z();
    print " Joystick x : " . $wii->get_wiimote_ext_nunchuk_joyx();
    print " y : " . $wii->get_wiimote_ext_nunchuk_joyy();
    print "\n-------------------\n";

    if ( $wii->get_wiimote_keys_home ) {
        $wii->wiimote_disconnect();
    }
    if ( $wii->get_wiimote_keys_up ) {
        print "\n UP \n";
    }
    if ( $wii->get_wiimote_keys_down ) {
        print "\n DOWN \n";
    }
    if ( $wii->get_wiimote_keys_left ) {
        print "\n LEFT \n";
    }
    if ( $wii->get_wiimote_keys_right ) {
        print "\n RIGHT \n";
    }
    if ( $wii->get_wiimote_keys_a ) {
        print "\n A KEY \n";
    }
    if ( $wii->get_wiimote_keys_b ) {
        print "\n B KEY \n";
    }
    if ( $wii->get_wiimote_keys_1 ) {
        print "\n 1 KEY \n";
    }
    if ( $wii->get_wiimote_keys_2 ) {
        print "\n 2 KEY \n";
    }
    if ( $wii->get_wiimote_keys_minus ) {
        print "\n MINUS KEY \n";
    }
    if ( $wii->get_wiimote_keys_plus ) {
        print "\n PLUS KEY \n";
    }
}
