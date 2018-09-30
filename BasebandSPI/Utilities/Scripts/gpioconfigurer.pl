use strict;

# Default value, 0 = write LOW`, 1 = write HIGH, 2 = toggle
my $kDefaultValue = 1;

# Port 6, Pin 7, Function 1 is GPIO3 - WLAN_RESET
# Default port
my $kDefaultPort = 6;
# Default Pin
my $kDefaultPin = 7;
# Default Function
my $kDefaultFunction = 1;

# registers
my $kPDAT0 = 0x3E400004;
my $kFSEL  = 0x3E400320;

# mem access
my $kMemAccess = "RegisterAccess";

# ===================================
sub readRegister
{
	my $r = shift;

	my $cmd = sprintf( "%s 0x%08X |", $kMemAccess, $r );
	#print( "$cmd" );
	
	open ( FD, $cmd );
	my $resp = <FD>;
	chomp $resp;

	my @tmp = split( / /, $resp );
	$_[0] = hex( $tmp[0] );
	close ( FD );
}

sub writeRegister
{
	my $r = shift;
	my $v = shift;

	my $cmd = sprintf( "%s 0x%08X 0x%08X", $kMemAccess, $r, $v );
	printf( "Using command %s\n", $cmd );
	system( $cmd );
}

sub writeRegisterMasked
{
	my $r = shift;
	my $v = shift;
	my $m = shift;

	my $cmd = sprintf( "%s 0x%08X 0x%08X 0x%08X", $kMemAccess, $r, $v, $m );
	printf( "Using command %s\n", $cmd );
	system( $cmd );
}

sub printUsage
{
	printf( "Options:\n");
	printf( "-p port  : Set the port, default is %s\n", $kDefaultPort );
	printf( "-n pin   : Set the pin, default is %s\n", $kDefaultPin );
	printf( "-f func  : Set the function, default is %s\n", $kDefaultFunction );
	printf( "-v value : Set the value, default is %s. 0 means write LOW, 1 means write HIGH, 2 means toggle.\n", $kDefaultValue );
}

my $port 	= $kDefaultPort;
my $pin 	= $kDefaultPin;
my $value 	= $kDefaultValue;
my $func  	= $kDefaultFunction;

my $argc = $#ARGV + 1;

for( my $i = 0; $i < $argc; $i += 2 )
{
	my $c = @ARGV[$i];

	my $next = @ARGV[$i+1];
	if ( $c eq "-p" )
		{
			$port = int( $next );

			if ( $port < 0 || $port > 24 ) 
			{
				print "Port $port must be between 0 and 24 inclusive\n";
				printUsage();
				exit();
			}
		}
	elsif ( $c eq "-f" )
		{
			$func = int( $next );

			if ( $func < 0 || $func > 7 )
			{
				print "Function $func must be between 0 and 7 inclusive.\n";
				printUsage();
				exit();
			}
		}
	elsif ( $c eq "-n" )
		{
			$pin = int( $next );

			if ( $pin < 0 || $pin > 15 )
			{
				print "Pin $pin must be between 0 and 15 inclusive.\n";
				printUsage();
				exit();
			}
		}
	elsif ( $c eq  "-v" )
		{
			$value = int( $next );
			if ( $value < 0 || $value > 2 )
			{
				print( "Value $value must be 0, 1 or 2.\n" );
				printUsage();
				exit();
			}
		}
	elsif ( $c eq "-h" )
		{
			printUsage();
			exit();
		}
	else
		{
			print "Unrecognized option $c\n";
			printUsage();
			exit();
		}
}

my $reg;
my $shift;
my $mask;
my $tmp;

printf( "Writing %d to GPIO %d.%d, Function %d\n", $value, $port, $pin, $func );

if ( $value == 2 )
{
	print( "toggle not supported yet\n" );
	exit();
}

$tmp		= ($port << 16) 
			| ($pin << 8 );

$reg 		=  $kFSEL;
if ( $func == 1 )
{
	$tmp = $tmp | $value | 0xE;
	writeRegister( $reg, $tmp );
}
else
{
	$tmp = $tmp | $func;
	writeRegister( $reg, $tmp );

	$reg = $kPDAT0 + 0x20*$port;

	$tmp = $value << $pin;
	my $mask = 1 << $pin;
	writeRegisterMasked( $reg, $tmp, $mask );
}
