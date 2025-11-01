#!/usr/bin/env python3
"""
Policy Switching Configuration Script

Enables and configures policy switching parameters on the Crazyflie.
Use this after flashing firmware with policy switching support.

Usage:
    python configure_policy_switching.py --enable --threshold 0.5
    python configure_policy_switching.py --disable
    python configure_policy_switching.py --status
"""

import argparse
import logging
import sys
import time

try:
    import cflib.crtp
    from cflib.crazyflie import Crazyflie
    from cflib.crazyflie.syncCrazyflie import SyncCrazyflie
except ImportError:
    print("Error: cflib not installed. Install with: pip install cflib")
    sys.exit(1)

# Setup logging
logging.basicConfig(level=logging.WARNING)

# Default URI - adjust for your Crazyflie
DEFAULT_URI = 'radio://0/80/2M'

def configure_policy_switching(uri, enable=True, threshold=0.5):
    """Configure policy switching parameters on the Crazyflie."""
    
    print(f"Connecting to {uri}...")
    cflib.crtp.init_drivers()
    
    with SyncCrazyflie(uri, cf=Crazyflie(rw_cache='./cache')) as scf:
        cf = scf.cf
        
        print("✓ Connected to Crazyflie")
        
        # Set policy switching enable
        cf.param.set_value('rlt.ps_enable', 1 if enable else 0)
        time.sleep(0.1)
        
        if enable:
            # Set threshold
            cf.param.set_value('rlt.ps_thresh', threshold)
            time.sleep(0.1)
            
            print(f"✓ Policy switching ENABLED")
            print(f"✓ Threshold set to {threshold}m")
        else:
            print("✓ Policy switching DISABLED")
        
        # Verify settings
        time.sleep(0.5)
        print("\nCurrent Configuration:")
        
        # Request parameter values
        cf.param.request_param_update('rlt.ps_enable')
        cf.param.request_param_update('rlt.ps_thresh')
        time.sleep(0.5)
        
        ps_enable = cf.param.get_value('rlt.ps_enable')
        ps_thresh = cf.param.get_value('rlt.ps_thresh')
        
        print(f"  ps_enable: {ps_enable} (0=off, 1=on)")
        print(f"  ps_thresh: {ps_thresh}m")
        
        if enable:
            print("\nPolicy Switching Active:")
            print(f"  Distance > {ps_thresh}m → Navigation actor")
            print(f"  Distance < {ps_thresh}m → Hover actor")
            print("  Target: (0.0, 1.2, 0.0)")
        
        print("\n✓ Configuration complete!")

def check_status(uri):
    """Check current policy switching status."""
    
    print(f"Connecting to {uri}...")
    cflib.crtp.init_drivers()
    
    with SyncCrazyflie(uri, cf=Crazyflie(rw_cache='./cache')) as scf:
        cf = scf.cf
        
        print("✓ Connected to Crazyflie\n")
        
        # Request parameter values
        cf.param.request_param_update('rlt.ps_enable')
        cf.param.request_param_update('rlt.ps_thresh')
        time.sleep(0.5)
        
        ps_enable = cf.param.get_value('rlt.ps_enable')
        ps_thresh = cf.param.get_value('rlt.ps_thresh')
        
        print("Policy Switching Status:")
        print(f"  Enable: {ps_enable} {'✓ ON' if ps_enable else '✗ OFF'}")
        print(f"  Threshold: {ps_thresh}m")
        
        if ps_enable:
            print(f"\n  Distance > {ps_thresh}m → Navigation Actor (obstacle avoidance)")
            print(f"  Distance < {ps_thresh}m → Hover Actor (stable hovering)")
            print("  Target Position: (0.0, 1.2, 0.0)")
        else:
            print("\n  Using single actor (navigation)")

def main():
    parser = argparse.ArgumentParser(
        description='Configure policy switching on Crazyflie',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  Enable with default threshold (0.5m):
    python configure_policy_switching.py --enable
  
  Enable with custom threshold:
    python configure_policy_switching.py --enable --threshold 0.6
  
  Disable policy switching:
    python configure_policy_switching.py --disable
  
  Check current status:
    python configure_policy_switching.py --status
  
  Custom URI:
    python configure_policy_switching.py --enable --uri radio://0/80/2M/E7E7E7E7E7
        """
    )
    
    parser.add_argument('--uri', type=str, default=DEFAULT_URI,
                        help=f'Crazyflie URI (default: {DEFAULT_URI})')
    parser.add_argument('--enable', action='store_true',
                        help='Enable policy switching')
    parser.add_argument('--disable', action='store_true',
                        help='Disable policy switching')
    parser.add_argument('--threshold', type=float, default=0.5,
                        help='Distance threshold in meters (default: 0.5)')
    parser.add_argument('--status', action='store_true',
                        help='Check current configuration')
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.enable and args.disable:
        print("Error: Cannot specify both --enable and --disable")
        sys.exit(1)
    
    if not (args.enable or args.disable or args.status):
        print("Error: Must specify --enable, --disable, or --status")
        parser.print_help()
        sys.exit(1)
    
    if args.threshold < 0.1 or args.threshold > 2.0:
        print("Warning: Threshold outside recommended range (0.1-2.0m)")
    
    try:
        if args.status:
            check_status(args.uri)
        else:
            configure_policy_switching(
                args.uri, 
                enable=args.enable, 
                threshold=args.threshold
            )
    except Exception as e:
        print(f"\nError: {e}")
        print("\nTroubleshooting:")
        print("  1. Ensure Crazyflie is powered on")
        print("  2. Check radio dongle connection (lsusb | grep Crazyradio)")
        print("  3. Verify URI is correct")
        print("  4. Try: sudo python configure_policy_switching.py ...")
        sys.exit(1)

if __name__ == '__main__':
    main()
