#!/bin/bash

while true; do
  clear
  echo "======================================================="
  echo "              LATHE POSITION MONITOR"
  echo "======================================================="
  echo
  
  # Lathe coordinates
  X_POS=$(halcmd getp lathe.position_x)
  Z_POS=$(halcmd getp lathe.position_z)
  
  # RP2040 encoder properties
  CONNECTED=$(halcmd getp rp2040-encoder.0.connected)
  TEST_MODE=$(halcmd getp rp2040-encoder.0.test-mode)
  
  # Encoder 0 (Z-axis) properties
  ENC0_POS=$(halcmd getp rp2040-encoder.0.position-0)
  ENC0_SCALE=$(halcmd getp rp2040-encoder.0.scale-0)
  ENC0_OFFSET=$(halcmd getp rp2040-encoder.0.offset-0)
  ENC0_RESET=$(halcmd getp rp2040-encoder.0.reset-0)
  ENC0_SET_POS=$(halcmd getp rp2040-encoder.0.set-position-0)
  
  # Encoder 1 (X-axis) properties
  ENC1_POS=$(halcmd getp rp2040-encoder.0.position-1)
  ENC1_SCALE=$(halcmd getp rp2040-encoder.0.scale-1)
  ENC1_OFFSET=$(halcmd getp rp2040-encoder.0.offset-1)
  ENC1_RESET=$(halcmd getp rp2040-encoder.0.reset-1)
  ENC1_SET_POS=$(halcmd getp rp2040-encoder.0.set-position-1)
  
  echo "  LATHE COORDINATES:"
  printf "    X-Axis: %10.3f mm\n" "$X_POS"
  printf "    Z-Axis: %10.3f mm\n" "$Z_POS"
  
  echo
  echo "  RP2040 ENCODER STATUS:"
  printf "    Connected: %s\n" "$CONNECTED"
  printf "    Test Mode: %s\n" "$TEST_MODE"
  
  echo
  echo "  ENCODER 0 (Z-AXIS):"
  printf "    Position:     %10.3f mm\n" "$ENC0_POS"
  printf "    Scale:        %10.6f mm/count\n" "$ENC0_SCALE"
  printf "    Offset:       %10.3f mm\n" "$ENC0_OFFSET"
  printf "    Reset:        %s\n" "$ENC0_RESET"
  printf "    Set Position: %10.3f mm\n" "$ENC0_SET_POS"
  
  echo
  echo "  ENCODER 1 (X-AXIS):"
  printf "    Position:     %10.3f mm\n" "$ENC1_POS"
  printf "    Scale:        %10.6f mm/count\n" "$ENC1_SCALE"
  printf "    Offset:       %10.3f mm\n" "$ENC1_OFFSET"
  printf "    Reset:        %s\n" "$ENC1_RESET"
  printf "    Set Position: %10.3f mm\n" "$ENC1_SET_POS"
  
  echo
  echo "======================================================="
  echo "Press Ctrl+C to exit"
  
  sleep 0.1
done