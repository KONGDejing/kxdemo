import sys
import os
import platform

import re
import time
import enum
import shutil
import glob

import queue
import zipfile
import subprocess
import threading
from ftplib import FTP

import serial
import serial.tools.list_ports

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QApplication, 
    QWidget, 
    QComboBox, 
    QPushButton, 
    QHBoxLayout,
    QVBoxLayout,
    QMessageBox,
    QProgressBar,
    QTextEdit
)


app = QApplication()


class SystemType(enum.Enum):
    WIN64 = "win64"
    MACOS = "macos"
    LINUX_AMD64 = "linux-amd64"
    LINUX_ARM64 = "linux-arm64"
    LINUX_ARM32 = "linux-arm32"


def create_ftp():
    ftp = FTP(
        host="101.35.120.126",
        user="kxdev",
        passwd="tdH3ce84DJey"
    )
    ftp.set_debuglevel(2)
    return ftp


def get_system_type():
    if sys.platform == 'win32':
        return SystemType.WIN64
    elif sys.platform == 'darwin':
        return SystemType.MACOS
    elif sys.platform == 'linux':
        if platform.machine() == "x86_64":
            return SystemType.LINUX_AMD64
        elif platform.machine() == "aarch64":
            return SystemType.LINUX_ARM64
        elif platform.machine() == "armv7l":
            return SystemType.LINUX_ARM32
    else:
        return SystemType.WIN64


def pipe_read_thread(layout: "MainWindow"):
    while True:
        if layout.pipe is not None:
            if layout.pipe.stdout is not None:
                if layout.pipe.stdout.readable():
                    line = layout.pipe.stdout.readline().decode().strip()
                    if len(line) > 0:
                        layout.pipe_queue.put(line)
                        if 'Writing at' in line:
                            p = re.findall(r'(\d*) %', line)[0]
                            layout.pipe_queue.put(int(p))
        time.sleep(0.1)


def get_esptool_path():
    # 创建储存位置
    store_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "esptool"
    )
    results = glob.glob(store_path + "/*/esptool*")
    if len(results) == 0:
        return None
    else:
        return results[0]


def download_esptool():
    # 创建储存位置
    store_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "esptool"
    )
    if not os.path.exists(store_path):
        os.mkdir(store_path)
    # 获取设备类型
    system_type = get_system_type()
    # 下载esptool工具包
    esptool_filename = f"esptool-v4.8.1-{system_type.value}.zip"
    esptool_store_path = os.path.join(store_path, "esptool.zip")
    if not os.path.exists(esptool_store_path):
        print(f"esptool not found {esptool_store_path}")
        print(f"download esptool from ftp: {esptool_filename}")
        # 创建ftp客户端
        ftp = create_ftp()
        # 下载
        ftp.retrbinary(
            f"RETR {esptool_filename}", 
            open(os.path.join(store_path, "esptool.zip"), "wb").write)
        # 退出ftp客户端
        ftp.quit()
    else:
        print(f"esptool found {esptool_store_path}")
    # 解压esptool工具包
    with zipfile.ZipFile(esptool_store_path, 'r') as zip_ref:
        zip_ref.extractall("esptool")


def flash_cmd(port: str) -> str:
    return (
        f"{get_esptool_path()} -p {port} "
        f"-b 460800 "
        f"--before default_reset "
        f"--after hard_reset "
        f"--chip esp32s3 "
        f"write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB "
        f"0x0 bootloader/bootloader.bin "
        f"0x20000 kx-box3.bin "
        f"0x8000 partition_table/partition-table.bin "
        f"0x16000 ota_data_initial.bin  "
        f"0xa55000 srmodels/srmodels.bin "
        f"0x573000 storage.bin"
    )
    

def fast_flash_cmd(port: str) -> str:
    return (
        f"{get_esptool_path()} -p {port} "
        f"-b 460800 "
        f"--before default_reset "
        f"--after hard_reset "
        f"--chip esp32s3 "
        f"write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB "
        f"0x20000 kx-box3.bin "
        f"0x573000 storage.bin"
    )


def download_firmware(device_type: str="box"):
    zip_file_name = f"{device_type}.zip"
    ftp = create_ftp()
    # Download firmware zip file
    ftp.retrbinary(f"RETR {zip_file_name}", open(zip_file_name, "wb").write)
    print(f"Firmware zip file downloaded: {zip_file_name}")


def unzip_firmware(device_type: str):
    zip_file_name = device_type + '.zip'
    with zipfile.ZipFile(zip_file_name, 'r') as zip_ref:
        zip_ref.extractall()
        print(f"Firmware zip file extracted: {zip_file_name}")


def delete_firmware_files():
    file_list = [
        "bootloader/bootloader.bin",
        "kx-box3.bin",
        "partition_table/partition-table.bin",
        "ota_data_initial.bin",
        "srmodels/srmodels.bin",
        "storage.bin"
    ]

    for file in file_list:
        if os.path.exists(file):
            os.remove(file)
            print(f"Firmware file removed: {file}")

    if os.path.exists("bootloader"):
        shutil.rmtree("bootloader")
        
    if os.path.exists("partition_table"):
        shutil.rmtree("partition_table")
    
    if  os.path.exists("srmodels"):
        shutil.rmtree("srmodels")
    

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        
        self.setWindowTitle("固件下载工具")
        self.setFixedSize(640, 480)

        # flash_layout
        self.port_combox = QComboBox()
        
        self.board_combox = QComboBox()
        self.board_combox.addItem("box")
        self.board_combox.addItem("dnesp")
        
        self.refresh_button = QPushButton('1 刷新串口')
        self.refresh_button.clicked.connect(self.on_refresh_button_clicked)
        
        self.download_button = QPushButton('2 下载固件')
        self.download_button.clicked.connect(self.on_download_button_clicked)
        
        self.flash_button = QPushButton('3 首次烧录')
        self.flash_button.clicked.connect(self.on_flash_button_clicked)
        
        self.fast_flash_button = QPushButton('4 快速烧录')
        self.fast_flash_button.clicked.connect(self.on_fast_flash_button_clicked)
        
        self.flash_layout = QHBoxLayout()
        self.flash_layout.addWidget(self.board_combox)
        self.flash_layout.addWidget(self.port_combox)
        self.flash_layout.addWidget(self.refresh_button)
        self.flash_layout.addWidget(self.download_button)
        self.flash_layout.addWidget(self.flash_button)
        self.flash_layout.addWidget(self.fast_flash_button)
        
        # progress_bar
        self.progress_bar = QProgressBar()
        self.progress_bar.setRange(0, 100)
        
        # message_edit
        self.message_text_edit = QTextEdit()
        
        # main_layout
        self.main_layout = QVBoxLayout(self)
        
        self.main_layout.addLayout(self.flash_layout)
        self.main_layout.addWidget(self.progress_bar)
        self.main_layout.addWidget(self.message_text_edit)
        
        # timer
        self.timer = QTimer(self)
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update_info_message_by_queue)
        self.timer.start()
        
        # other tools
        self.pipe: subprocess.Popen = None
        self.pipe_queue = queue.Queue()
        
        self.backend_thread = threading.Thread(target=pipe_read_thread, args=(self,), daemon=True)
        self.backend_thread.start()
        
    def get_selected_port(self) -> str:
        return self.port_combox.currentText().split(' - ')[0]
        
    def show_info_message(self, text: str):
        QMessageBox.information(self, '提示', text)
    
    def on_refresh_button_clicked(self):
        port_list = list(serial.tools.list_ports.comports())
        self.port_combox.clear()
        for port in port_list:
            self.port_combox.addItem(f'{port.device} - {port.description}')
    
    def on_flash_button_clicked(self):
        if not os.path.exists("kx-box3.bin"):
            self.show_info_message("请先下载固件")
            return
        if self.pipe is None:
            self.pipe = subprocess.Popen(flash_cmd(self.get_selected_port()), shell=True, stdout=subprocess.PIPE)
            self.progress_bar.setValue(0)
            self.message_text_edit.clear()
            self.show_info_message("开始下载固件...")
        else:
            self.show_info_message("固件下载中...")
            
    def on_fast_flash_button_clicked(self):
        if not os.path.exists("kx-box3.bin"):
            self.show_info_message("请先下载固件")
            return
        if self.pipe is None:
            self.pipe = subprocess.Popen(fast_flash_cmd(self.get_selected_port()), shell=True, stdout=subprocess.PIPE)
            self.progress_bar.setValue(0)
            self.message_text_edit.clear()
            self.show_info_message("开始下载固件...")
        else:
            self.show_info_message("固件下载中...")
    
    def on_download_button_clicked(self):
        try:
            self.show_info_message("开始下载esptool...")
            download_esptool()
            self.show_info_message("开始下载固件...")
            download_firmware(self.board_combox.currentText())
            unzip_firmware(self.board_combox.currentText())
            self.show_info_message("固件下载成功")
        except Exception as e:
            print(e)
            self.show_info_message("固件下载失败")
    
    def update_info_message_by_queue(self):
        if self.pipe_queue.qsize() > 0:
            msg = self.pipe_queue.get()
            if isinstance(msg, int):
                self.progress_bar.setValue(msg)
            else:
                self.message_text_edit.append(msg)
        if self.pipe is not None:
            if self.pipe.poll() is not None:
                self.pipe = None
                if self.progress_bar.value() == 100:
                    self.show_info_message("固件下载成功")
                else:
                    self.show_info_message("固件下载失败")
    
if __name__ == '__main__':
    main_window = MainWindow()
    main_window.show()

    app.exec()
    
    delete_firmware_files()
    