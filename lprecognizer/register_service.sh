#!/bin/bash

#ln -s /opt/inex/lprecognizer/cfg/lprecognizer.service /etc/systemd/system/lprecognizer.service
#ln -s /etc/systemd/system/lprecognizer.service /etc/systemd/system/multi-user.target.wants/lprecognizer.service

systemctl enable /opt/inex/lprecognizer/cfg/lprecognizer.service
systemctl daemon-reload
#systemctl enable lprecognizer.service