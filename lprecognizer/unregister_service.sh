#!/bin/bash

systemctl stop lprecognizer.service
systemctl stop lpmanager.service

rm /etc/systemd/system/lprecognizer.service
rm /etc/systemd/system/multi-user.target.wants/lprecognizer.service
rm /etc/systemd/system/lpmanager.service
rm /etc/systemd/system/multi-user.target.wants/lpmanager.service

systemctl daemon-reload
