#!/usr/bin/env bash

set -eu

./xkbgen aegreek.kb "Greek (improved)" > ./generated/aegreek
./xkbgen ae.kb "English with IPA" > ./generated/ae
