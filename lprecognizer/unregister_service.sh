#!/bin/bash

systemctl stop lprecognizer.service

rm /etc/systemd/system/lprecognizer.service
rm /etc/systemd/system/multi-user.target.wants/lprecognizer.service

systemctl daemon-reload
