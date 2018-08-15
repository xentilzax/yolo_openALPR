sudo apt install curl libopencv-dev libcurl4-openssl-dev

#CUDA
sudo apt-get install nvidia-384 nvidia-modprobe
wget https://developer.nvidia.com/compute/cuda/9.0/Prod/local_installers/cuda_9.0.176_384.81_linux-run
chmod +x cuda_9.0.176_384.81_linux-run
./cuda_9.0.176_384.81_linux-run
sudo ./cuda-linux.9.0.176-22781540.run
sudo bash -c "echo /usr/local/cuda/lib64/ > /etc/ld.so.conf.d/cuda.conf"
sudo ldconfig

echo export CUDA_HOME=/usr/local/cuda-9.0 >> .bashrc 
echo export LD_LIBRARY_PATH=\${CUDA_HOME}/lib:\$LD_LIBRARY_PATH >> .bashrc
echo export PATH=\${CUDA_HOME}/bin:\${PATH} >> .bashrc

#it just for NVCC
#uncomment this if you want compile your sources with nvcc
#cp /etc/environment ~/environment
#sed -i -e 's/\:\/usr\/local\/cuda\/bin//g' ~/environment
#sed -i -e 's/PATH=\"/PATH=\"\/usr\/local\/cuda\/bin:/g' ~/environment
#sudo cp ~/environment /etc/
#sudo echo PATH=\${PATH}:/usr/local/cuda/bin >> ~/environment

#CUDnn


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