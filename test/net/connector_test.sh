#!/bin/bash
socat - TCP-LISTEN:9996 &
sleep 4
socat - TCP-LISTEN:9997 &
socat - TCP-LISTEN:9998 &
socat - TCP-LISTEN:9999 &
