use strict;

# PLL
my $kDefaultPLL = 1;

# power state
my $kDefaultPowerState = 0;

# base register
my $kBaseReg = 0x3C500020;

# mem access
my $kMemAccess = "RegisterAccess";

# CLKCON1
my $kCLKCON1 = 0x3C500004;

# ===================================

sub printUsage;

sub readRegister;

my $PLL 			= $kDefaultPLL;
my $powerState 		= $kDefaultPowerState;

my $argc = $#ARGV + 1;

for( my $i = 0; $i < $argc; $i+=2 )
{
	my $c = @ARGV[$i];

	my $next = @ARGV[$i+1];
	if ( $c eq "-p" )
		{
			$PLL = int( $next );

			if ( $PLL < 0 || $PLL > 3 ) 
			{
				print "PLL $PLL must be between 0 and 3 inclusive.\n";
				printUsage();
				exit();
			}
		}
	elsif ( $c eq "-s" )
		{
			$powerState = int( $next );

			if ( $powerState < 0 || $powerState > 2 )
			{
				print "Divider $powerState must be between 0 and 2 inclusive.\n";
				printUsage();
				exit();
			}
		}
	elsif ( $c eq  "-h" )
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


# According to JDC:
#				PLL.s		PCLK0.div/PCLK1.div
#  State 0: 	0			2
#  State 1: 	1			2
#  State 2: 	2			1
#

my $s = 0;
my $div = $1;

if ( $powerState == 0 )
{
	$s = 0;
	$div = 2;
}
elsif ( $powerState == 1 )
{
	$s = 1;
	$div = 2;
}
else
{
	$s = 2;
	$div = 1;
}

printf( "Using PowerState %d\n", $powerState );

my $value;
my $mask;

# read CLKCON1
$div = $div | ($div << 2 );

$mask = 0xF << 20;
$value = $div << 20;

# write CLKCON1
writeRegisterMasked( $kCLKCON1, $value, $mask );

# read PLL
my $reg = $kBaseReg + 4*$PLL;

readRegister( $reg, $value );

$mask = 0x7;

# write PLL
writeRegisterMasked( $reg, $s, $mask );

sub printUsage
{
	printf( "Options:\n");
	printf( "-s powerState  : Set the power state, default is %s\n", $kDefaultPowerState );
	printf( "-p PLL         : Set the PLL, default is %s\n", $kDefaultPLL );
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
