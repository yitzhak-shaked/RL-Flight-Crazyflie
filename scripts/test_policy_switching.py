#!/usr/bin/env python3
"""
Test script for policy switching feature.

Usage:
    python scripts/test_policy_switching.py --nav-actor <path> --hover-actor <path> [--threshold 0.3]

Example:
    python scripts/test_policy_switching.py \
        --nav-actor actors/position_to_position_at_2m_actor_2400000.h5 \
        --hover-actor actors/hover_actors/hoverActor_000000000300000.h5 \
        --threshold 0.3
"""

import asyncio
import websockets
import json
import argparse
import sys

async def send_command(uri, message):
    """Send a command to the UI WebSocket server."""
    try:
        async with websockets.connect(uri) as websocket:
            await websocket.send(json.dumps(message))
            print(f"✓ Sent: {message['data']['action']}")
            
            # Wait for any response
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=2.0)
                print(f"  Response: {response[:100]}")
            except asyncio.TimeoutError:
                print("  (No response received, which is normal)")
                
    except Exception as e:
        print(f"✗ Error: {e}")
        return False
    return True

async def test_policy_switching(args):
    """Test the policy switching feature."""
    uri = f"ws://{args.host}:{args.port}"
    
    print("=" * 70)
    print("POLICY SWITCHING TEST")
    print("=" * 70)
    print(f"Server: {uri}")
    print(f"Navigation Actor: {args.nav_actor}")
    print(f"Hover Actor: {args.hover_actor}")
    print(f"Threshold: {args.threshold}m")
    print("=" * 70)
    print()
    
    # Step 1: Start evaluation with navigation actor
    print("Step 1: Loading navigation actor...")
    success = await send_command(uri, {
        "channel": "evaluateActor",
        "data": {
            "action": "restart",
            "actorPath": args.nav_actor
        }
    })
    
    if not success:
        print("\n✗ Failed to load navigation actor. Is the UI server running?")
        print(f"  Try: OPENBLAS_NUM_THREADS=1 ./build/src/ui {args.host} {args.port}")
        return
    
    await asyncio.sleep(1)
    
    # Step 2: Enable policy switching
    print("\nStep 2: Enabling policy switching...")
    success = await send_command(uri, {
        "channel": "evaluateActor",
        "data": {
            "action": "enablePolicySwitching",
            "hoverActorPath": args.hover_actor,
            "threshold": args.threshold
        }
    })
    
    if not success:
        print("\n✗ Failed to enable policy switching")
        return
    
    print("\n" + "=" * 70)
    print("✓ Policy switching enabled successfully!")
    print("=" * 70)
    print("\nThe drone will now:")
    print(f"  • Use NAVIGATION actor when distance > {args.threshold}m from target")
    print(f"  • Use HOVER actor when distance < {args.threshold}m from target")
    print("\nWatch the UI to see the switching in action!")
    print("\nTo disable policy switching, run:")
    print("  python scripts/test_policy_switching.py --disable")

async def disable_policy_switching(args):
    """Disable policy switching."""
    uri = f"ws://{args.host}:{args.port}"
    
    print("=" * 70)
    print("DISABLING POLICY SWITCHING")
    print("=" * 70)
    
    success = await send_command(uri, {
        "channel": "evaluateActor",
        "data": {
            "action": "disablePolicySwitching"
        }
    })
    
    if success:
        print("\n✓ Policy switching disabled")
    else:
        print("\n✗ Failed to disable policy switching")

def main():
    parser = argparse.ArgumentParser(
        description="Test policy switching for RL-Flight-Crazyflie",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Enable policy switching with default threshold (0.3m)
  python scripts/test_policy_switching.py \\
      --nav-actor actors/position_to_position_at_2m_actor_2400000.h5 \\
      --hover-actor actors/hover_actors/hoverActor_000000000300000.h5

  # Enable with custom threshold
  python scripts/test_policy_switching.py \\
      --nav-actor actors/position_to_position_at_2m_actor_2400000.h5 \\
      --hover-actor actors/hover_actors/hoverActor_000000000300000.h5 \\
      --threshold 0.5

  # Disable policy switching
  python scripts/test_policy_switching.py --disable
        """
    )
    
    parser.add_argument("--host", default="localhost", help="UI server host (default: localhost)")
    parser.add_argument("--port", type=int, default=8080, help="UI server port (default: 8080)")
    parser.add_argument("--nav-actor", help="Path to navigation actor (e.g., actors/position_to_position_at_2m_actor_2400000.h5)")
    parser.add_argument("--hover-actor", help="Path to hover actor (e.g., actors/hover_actors/hoverActor_000000000300000.h5)")
    parser.add_argument("--threshold", type=float, default=0.3, help="Distance threshold for switching (default: 0.3m)")
    parser.add_argument("--disable", action="store_true", help="Disable policy switching")
    
    args = parser.parse_args()
    
    if args.disable:
        asyncio.run(disable_policy_switching(args))
    elif not args.nav_actor or not args.hover_actor:
        parser.print_help()
        print("\n✗ Error: Both --nav-actor and --hover-actor are required (unless using --disable)")
        sys.exit(1)
    else:
        asyncio.run(test_policy_switching(args))

if __name__ == "__main__":
    main()
