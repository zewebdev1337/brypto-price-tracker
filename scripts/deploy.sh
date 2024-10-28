#!/bin/bash

systemctl disable --user --now prices.service

if [ ! -d "build" ]; then
    mkdir build
fi


if [ ! -d ~/.config/systemd/user ]; then
    mkdir ~/.config/systemd/user
fi

cd build
qmake ../widget.pro
make
mv price-tracker /usr/local/bin/brypto-price-tracker
cd ..
rm -rf build

cat << 'EOF' > ~/.config/systemd/user/prices.service
[Unit]
Description=Brypto price tracker service. It starts after successfully connecting to the internet
After=network.target
Wants=network-online.target

[Service]
ExecStart=/usr/local/bin/brypto-price-tracker
WorkingDirectory=/usr/local/bin/
Restart=always
RestartSec=30

[Install]
WantedBy=default.target
EOF

systemctl enable --user --now prices.service

