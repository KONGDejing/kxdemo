import os
import sys
import zipfile
from ftplib import FTP

from datetime import datetime


base_path = "../build"


file_store_list = [
    "bootloader/bootloader.bin",
    "kx-box3.bin",
    "partition_table/partition-table.bin",
    "ota_data_initial.bin",
    "srmodels/srmodels.bin",
    "storage.bin"
]


file_list = [
    os.path.join(base_path, "bootloader/bootloader.bin"),
    os.path.join(base_path, "kx-box3.bin"),
    os.path.join(base_path, "partition_table/partition-table.bin"),
    os.path.join(base_path, "ota_data_initial.bin"),
    os.path.join(base_path, "srmodels/srmodels.bin"),
    os.path.join(base_path, "storage.bin")
]


if __name__ == "__main__":
    board_name = sys.argv[1]

    for file_path in file_list:
        if os.path.exists(file_path):
            print(f"File found: {file_path}")
        else:
            sys.exit(f"Error: File not found: {file_path}")

    zip_file_name = f"{board_name}.zip"

    with zipfile.ZipFile(zip_file_name, "w", zipfile.ZIP_DEFLATED) as zipf:
        for index, file_path in enumerate(file_list):
            zipf.write(file_path, file_store_list[index], compress_type=zipfile.ZIP_DEFLATED)
            # os.remove(file_path)
            print(f"File added to zip: {file_path}")
        print(f"Firmware zip file created: {zip_file_name}")

    # Upload to FTP server

    ftp = FTP(
        host="101.35.120.126",
        user="kxdev",
        passwd="tdH3ce84DJey"
    )

    ftp.set_debuglevel(2)

    with open(zip_file_name, "rb") as file:
        ftp.storbinary(f"STOR {zip_file_name}", file)
        print(f"Firmware zip file uploaded to FTP: {zip_file_name}")
    ftp.quit()
    os.remove(zip_file_name)

    print("Firmware zip file deleted")
    print("Firmware packaging completed successfully")
    print(f"Current date and time: {datetime.now()}")
    print(f"Base path: {base_path}")
    print(f"Board name: {board_name}")