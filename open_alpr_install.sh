#!/bin/bash

RELEASE_NAME=`cat /etc/lsb-release  | grep DISTRIB_CODENAME | sed 's/DISTRIB_CODENAME\s*=\s*//g'`
REPO_POSTFIX="/"
OS_NAME=""

if  [ -e "/etc/nv_tegra_release" ]; then

  # Check JetPack version and setup the correct repo
  if cat /etc/nv_tegra_release  | head -n 1 | grep "R28.*REVISION:\s*2."; then
    # We are running JetPack 3.2
    OS_NAME="Nvidia Jetson JetPack 3.2"
    DEB_REPO_PATH="deb https://deb.openalpr.com/jetson32${REPO_POSTFIX} jetson32 main"
  elif cat /etc/nv_tegra_release  | head -n 1 | grep "R28.*REVISION:\s*1.0"; then
    # We are running JetPack 3.1
    OS_NAME="Nvidia Jetson JetPack 3.1"
    DEB_REPO_PATH="deb https://deb.openalpr.com/jetson-xenial-commercial${REPO_POSTFIX} jetson-xenial main"
  elif cat /etc/nv_tegra_release  | head -n 1 | grep "R31.*"; then
    # We are running JetPack 3.1
    OS_NAME="Nvidia Jetson JetPack 4.0"
    DEB_REPO_PATH="deb https://deb.openalpr.com/jetson40${REPO_POSTFIX} jetson40 main"
  else
    echo "Could not determine JetPack version"
    exit 1
  fi

elif [ "$RELEASE_NAME" = "trusty" ]; then
  OS_NAME="Ubuntu Trusty"
  DEB_REPO_PATH="deb https://deb.openalpr.com/commercial${REPO_POSTFIX} trusty main"
elif [ "$RELEASE_NAME" = "xenial" ]; then
  OS_NAME="Ubuntu Xenial"
  DEB_REPO_PATH="deb https://deb.openalpr.com/xenial-commercial${REPO_POSTFIX} xenial main"
elif [ "$RELEASE_NAME" = "bionic" ]; then
  OS_NAME="Ubuntu Bionic"
  DEB_REPO_PATH="deb https://deb.openalpr.com/bionic${REPO_POSTFIX} bionic main"
else
  echo "Unable to determine Ubuntu release version.  OpenALPR supports installation on Ubuntu 16.04 and 18.04"
  exit 1
fi

if [[ $EUID -eq 0 ]]; then
    # Running as root
    SUDO_COMMAND=""
else
    # Not running as root
    SUDO_COMMAND="sudo"

    # Check if sudo is available
    which sudo >/dev/null 2>&1
    if [ "$?" == "1" ];
    then
        echo "sudo command not found.  You must have this installed in order to continue."
        echo "You can install sudo with the command:"
        echo "  apt-get install sudo."
        exit 1
    fi

fi



INFO_QUALIFIER=""


declare -a OPTIONS=(\
install_webserver   "Install OpenALPR $INFO_QUALIFIER Web Server" \
install_agent       "Install OpenALPR $INFO_QUALIFIER Agent" \
install_sdk         "Install OpenALPR $INFO_QUALIFIER Developer SDK" \
install_nvidia      "Install OpenALPR Nvidia Acceleration" \
upgrade_software    "Upgrade to the latest OpenALPR software" \
uninstall_agent     "Uninstall OpenALPR Agent" \
uninstall_webserver "Uninstall OpenALPR Web Server" \
)


DEBUG=0

ALL_YES=0
if [ "$1" == "-y" ];
then
    ALL_YES=1
fi

prompt_confirm() {

  if [ "$ALL_YES" -eq 1 ];
  then
    return 0;
  fi

  while true; do
    read -r -n 1 -p "${1:-Continue?} [y/n]: " REPLY
    case $REPLY in
      [yY]) echo ; return 0 ;;
      [nN]) echo ; return 1 ;;
      *) printf " \033[31m %s \n\033[0m" "invalid input"
    esac 
  done  
}

append_repo_setup()
{
    COMMANDS=$1
    COMMANDS+=("curl -L https://deb.openalpr.com/openalpr.gpg.key | $SUDO_COMMAND apt-key add -")
    COMMANDS+=("echo '$DEB_REPO_PATH' | $SUDO_COMMAND tee /etc/apt/sources.list.d/openalpr.list")
}

run_commands_with_confirm()
{
    echo "Detected Operating System: ${OS_NAME}"
    echo "OpenALPR will execute the following commands to install the software"
    echo ""
    echo "--------------------------------------------------------------------"
    for cmd in "${COMMANDS[@]}"
    do
        echo "    $cmd"
    done
    echo ""
    echo "--------------------------------------------------------------------"
    echo ""
    prompt_confirm

    if [ $? -ne 0 ]; then
        echo "Installation canceled."
        exit 1
    fi

    for cmd in "${COMMANDS[@]}"
    do
        eval $cmd
        if [ $? -ne 0 ]; then
            echo "Installation failed."
            exit 1
        fi
    done
}

do_task()
{
    task=$1
    
    if [ "$DEBUG" -eq 1 ];
    then
        echo "Bypassing task $task due to debug enabled"
        return 0
    else
        echo ""
        echo "-------------------------------"
        echo "Executing task: $task"
        echo "-------------------------------"
    fi
    
    task=$1
    
    COMMANDS=()

    if [ "$task" == "install_agent" ];
    then
        echo "Installing OpenALPR Agent"
        append_repo_setup $COMMANDS
        COMMANDS+=("$SUDO_COMMAND apt-get update; $SUDO_COMMAND apt-get install -o apt::install-recommends=true -y openalpr openalpr-daemon openalpr-link")
        if [[ $OS_NAME == Nvidia* ]];
        then
            COMMANDS+=("$SUDO_COMMAND apt-get -y install openalprgpu")
            COMMANDS+=("$SUDO_COMMAND sed -i -n -e '/^hardware_acceleration/!p' -e '\$ahardware_acceleration = 1' /etc/openalpr/openalpr.conf")
            COMMANDS+=("$SUDO_COMMAND sed -i -n -e '/^gpu_batch_size/!p' -e '\$agpu_batch_size = 5' /etc/openalpr/openalpr.conf")
        fi

        COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        run_commands_with_confirm $COMMANDS

        DESKTOP_DIR=$(eval echo "~$USER/Desktop")
        DESKTOP_SCRIPT="openalpr-agentconfig.desktop"
        if [ -d "$DESKTOP_DIR" ]; then
          curl "https://deb.openalpr.com/installer_files/${DESKTOP_SCRIPT}" -o "$DESKTOP_DIR/${DESKTOP_SCRIPT}" && \
          chmod +x "$DESKTOP_DIR/${DESKTOP_SCRIPT}"
        fi

    fi
    
    if [ "$task" == "install_sdk" ];
    then
        echo "Installing OpenALPR Agent"
        append_repo_setup $COMMANDS
        COMMANDS+=("$SUDO_COMMAND apt-get update; $SUDO_COMMAND apt-get install -o apt::install-recommends=true -y openalpr libopenalpr-dev libalprstream-dev python-openalpr openalpr-video")
        if [[ $OS_NAME == Nvidia* ]];
        then
            COMMANDS+=("$SUDO_COMMAND apt-get -y install openalprgpu")
            COMMANDS+=("$SUDO_COMMAND sed -i -n -e '/^hardware_acceleration/!p' -e '\$ahardware_acceleration = 1' /etc/openalpr/openalpr.conf")
            COMMANDS+=("$SUDO_COMMAND sed -i -n -e '/^gpu_batch_size/!p' -e '\$agpu_batch_size = 5' /etc/openalpr/openalpr.conf")
        fi
        COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        run_commands_with_confirm $COMMANDS


    fi

    if [ "$task" == "install_webserver" ];
    then

        WEB_SERVER_INSTALLED=0
        $SUDO_COMMAND dpkg --get-selections | grep -e "^openalpr-web\s*install" >/dev/null 2>&1
        if [ "$?" == "0" ];
        then
            WEB_SERVER_INSTALLED=1
        fi

        if [ "$WEB_SERVER_INSTALLED" != "1" ];
        then
            # Check if ports are open first
            $SUDO_COMMAND lsof -i -P -n | grep LISTEN | grep -e ":80\s" > /dev/null 2>&1
            HTTP_IN_USE=$?
            $SUDO_COMMAND lsof -i -P -n | grep LISTEN | grep -e ":443\s" > /dev/null 2>&1
            HTTPS_IN_USE=$?
            if [ "$HTTP_IN_USE" == "0" ] || [ "$HTTPS_IN_USE" == "0" ];
            then
                echo "--------------------------------------------------------------------------"
                echo "                             Error"
                echo "--------------------------------------------------------------------------"
                echo "Detected port 80 or port 443 is already in use."
                echo "Please uninstall software using these ports in order to install OpenALPR"
                echo "Install cannot continue..."
                echo ""
                exit 1
            fi
        fi


        echo "Installing OpenALPR Web Server"
        append_repo_setup $COMMANDS
        COMMANDS+=("$SUDO_COMMAND apt-get update; $SUDO_COMMAND apt-get install -o apt::install-recommends=true -y openalpr-web")
        COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        
        # Only do the create-database script on fresh install
        if [ "$WEB_SERVER_INSTALLED" == "0" ];
        then
            COMMANDS+=("$SUDO_COMMAND openalpr-web-createdb")
        fi

        COMMANDS+=("$SUDO_COMMAND /etc/init.d/openalpr-web restart")
        run_commands_with_confirm $COMMANDS

        DESKTOP_DIR=$(eval echo "~$USER/Desktop")
        DESKTOP_SCRIPT="openalpr-weblink.desktop"
        if [ -d "$DESKTOP_DIR" ]; then
          curl "https://deb.openalpr.com/installer_files/${DESKTOP_SCRIPT}" -o "$DESKTOP_DIR/${DESKTOP_SCRIPT}" && \
          chmod +x "$DESKTOP_DIR/${DESKTOP_SCRIPT}"
        fi
    fi

    if [ "$task" == "upgrade_software" ];
    then
        append_repo_setup $COMMANDS
        COMMANDS+=("$SUDO_COMMAND apt-get update; $SUDO_COMMAND apt-get dist-upgrade -y -o Dir::Etc::sourcelist=\"sources.list.d/openalpr.list\"     -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0"-upgrade")
        COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        run_commands_with_confirm $COMMANDS
    fi
    
    if [ "$task" == "install_nvidia" ];
    then
        append_repo_setup $COMMANDS
        COMMANDS+=("$SUDO_COMMAND apt-get update; $SUDO_COMMAND apt-get -y install openalpr openalprgpu")
        COMMANDS+=("$SUDO_COMMAND sed -i -n -e '/^hardware_acceleration/!p' -e '\$ahardware_acceleration = 1' /etc/openalpr/openalpr.conf")
        COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        run_commands_with_confirm $COMMANDS
    fi


    if [ "$task" == "register_agent" ];
    then

        WEB_SERVER_INSTALLED=0
        $SUDO_COMMAND dpkg -s openalpr-web >/dev/null 2>&1
        if [ "$?" == "0" ];
        then
            WEB_SERVER_INSTALLED=1
        fi

        $SUDO_COMMAND dpkg -s openalpr-link >/dev/null 2>&1
        if [ "$?" == "0" ];
        then

            WEBSERVER_HOST="https://cloud.openalpr.com"
            if [ "$WEB_SERVER_INSTALLED" == "1" ];
            then
                WEBSERVER_HOST=https://localhost
            fi

            echo ""
            echo "Agent Registration"
            prompt_confirm "Do you wish to register the agent with $WEBSERVER_HOST? " 
            REGISTER_AGENT=$?
            echo ""

            if [[ $REGISTER_AGENT == "0" ]]
            then
                # loop until exit code is 0
                until $SUDO_COMMAND alprlink-register -w "$WEBSERVER_HOST"
                do
                    echo "Please try again..."
                done
            fi


        fi
    fi

    if [ "$task" == "uninstall_agent" ];
    then
        echo "Removing OpenALPR Agent"
        COMMANDS+=("$SUDO_COMMAND apt-get purge -y openalpr-daemon libopenalpr-dev openalpr-link")
        if [ -e "/etc/apt/sources.list.d/openalpr.list" ]; then
          COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        fi
        run_commands_with_confirm $COMMANDS
    fi

    if [ "$task" == "uninstall_webserver" ];
    then
        echo "Removing OpenALPR Web Server"
        COMMANDS+=("$SUDO_COMMAND openalpr-web-resetdb || true")
        COMMANDS+=("$SUDO_COMMAND apt-get purge -y openalpr-web")
        COMMANDS+=("$SUDO_COMMAND apt-get autoremove -y")
        if [ -e "/etc/apt/sources.list.d/openalpr.list" ]; then
          COMMANDS+=("$SUDO_COMMAND rm /etc/apt/sources.list.d/openalpr.list")
        fi
        run_commands_with_confirm $COMMANDS
    fi
}





# Trap CTRL-C (Cancel the entire script if CTRL-C)
trap ctrl_c INT

function ctrl_c() {
    exit 1
}





PREREQS=""


# Check if dialog is already installed
$SUDO_COMMAND dpkg -s dialog >/dev/null 2>&1
if [ "$?" == "1" ];
then
    PREREQS=$PREREQS" dialog"
fi

# Check if apt-transport-https is installed
$SUDO_COMMAND dpkg -s apt-transport-https  >/dev/null 2>&1
if [ "$?" == "1" ];
then
    PREREQS=$PREREQS" apt-transport-https"
fi

# Check if apt-transport-https is installed
$SUDO_COMMAND dpkg -s gnupg  >/dev/null 2>&1
if [ "$?" == "1" ];
then
    PREREQS=$PREREQS" gnupg"
fi

# Check if prereq is installed
$SUDO_COMMAND dpkg -s lsof  >/dev/null 2>&1
if [ "$?" == "1" ];
then
    PREREQS=$PREREQS" lsof"
fi

if [ -z "$PREREQS" ];
then
    echo "Prerequisites satisfied"
else
    echo "$PREREQS"
    $SUDO_COMMAND add-apt-repository universe
    $SUDO_COMMAND apt-get update; $SUDO_COMMAND apt-get install -y $PREREQS  < /dev/null
fi


DIALOG_ARGS='--checklist "Choose OpenALPR Install Tasks (Press SPACE to select):" 18 72 10 '

end_array=$[${#OPTIONS[@]}-1]
for i in $(seq 0 2 $end_array)
do
    #echo $i
    DIALOG_ARGS="$DIALOG_ARGS '${OPTIONS[$i]}' '${OPTIONS[$i+1]}' off "

done

TEMP_CHOICE_RESULTS_FILE=/tmp/choice_results
$SUDO_COMMAND rm $TEMP_CHOICE_RESULTS_FILE; touch $TEMP_CHOICE_RESULTS_FILE

get_install_selection()
{

    echo dialog ${DIALOG_ARGS} | bash  2>$TEMP_CHOICE_RESULTS_FILE
 
}


while [ true ]
do
    
    get_install_selection
    if [ $? -ne 0 ]; then
        clear
        echo "Installation canceled."
        break
    fi

    clear
    choices=`cat $TEMP_CHOICE_RESULTS_FILE`
    echo "" > $TEMP_CHOICE_RESULTS_FILE
    
    for choice in $choices
    do
        # remove prefix/suffice quote (") characters
        choice="${choice%\"}"
        choice="${choice#\"}"
        
        # Do it
        #echo "Your choice: $choice"
        do_task $choice
    done
    
    # If they chose at least one thing
    if [ ${#choice[@]} -gt 0 ]; then
        do_task register_agent
    else
        continue
    fi
    

    echo "Installation Complete"
    break

done



