#!/usr/bin/env bash

set -eu

./xkbgen aegreek.kb iso-105 "Greek (improved)" > ./generated/aegreek
./xkbgen ae.kb iso-105 "English with IPA" > ./generated/ae
