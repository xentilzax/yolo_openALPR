sudo apt install curl libopencv-dev libcurl4-openssl-dev

#CUDA
sudo apt-get install nvidia-384 nvidia-modprobe

wget https://developer.nvidia.com/compute/cuda/9.2/Prod2/local_installers/cuda-repo-ubuntu1604-9-2-local_9.2.148-1_amd64
mv cuda-repo-ubuntu1604-9-2-local_9.2.148-1_amd64 cuda-repo-ubuntu1604-9-2-local_9.2.148-1_amd64.deb
sudo dpkg -i cuda-repo-ubuntu1604-9-2-local_9.2.148-1_amd64.deb
sudo apt-key add /var/cuda-repo-9-2-local/7fa2af80.pub
sudo apt-get update
sudo apt-get install cuda


#CUDnn
#wget https://developer.nvidia.com/compute/machine-learning/cudnn/secure/v7.2.1/prod/9.0_20180806/Ubuntu16_04-x64/libcudnn7-dev_7.2.1.38-1_cuda9.0_amd64

# install open_ALPR
#bash <(curl -s https://deb.openalpr.com/install)
    #add repo
    curl -L https://deb.openalpr.com/openalpr.gpg.key | sudo apt-key add -
    echo 'deb https://deb.openalpr.com/xenial-commercial/ xenial main' | sudo tee /etc/apt/sources.list.d/openalpr.list
    sudo apt-get update

    #SDK dev
    sudo apt-get install -y openalpr libopenalpr-dev libalprstream-dev python-openalpr

    #GPU
    sudo apt-get -y install openalpr openalprgpu
    sudo sed -i -n -e '/^hardware_acceleration/!p' -e ' = 1' /etc/openalpr/openalpr.conf
    
    #update open_ALPR
    sudo apt-get -y dist-upgrade

    #remove repoq
    sudo rm /etc/apt/sources.list.d/openalpr.list
    
echo "---------------------------------------------"
echo "           NEED REBOOT SYSTEM                "
echo "---------------------------------------------"

#sudo reboot