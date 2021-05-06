#!/bin/sh
cargo build --release
cargo strip
mv ./target/release/png_parser ../
