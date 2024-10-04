# ESP-IDF Setup for ADF Integration

## Prerequisites
- ESP-IDF v4.4.6 is recommended.

## Installation Steps

1. Open Visual Studio Code (VS Code).

2. Press `F1` and select `Install-ADF`.
![image](https://github.com/eSpark-Consultants/simonlore-voiceassistant-firmware/assets/106241042/6d2675ee-844d-4f92-a14f-a57658dcc6c4)

3. Click on "Choose a container directory."
![image](https://github.com/eSpark-Consultants/simonlore-voiceassistant-firmware/assets/106241042/1322a96c-f0ba-46a4-9770-842d356dda3a)


4. Select "Use GitHub."

5. Choose the path where ESP-IDF is installed (Example: `C:\Users\{Username}\esp`).

6. A progress bar will be shown at the bottom right of the screen.
![image](https://github.com/eSpark-Consultants/simonlore-voiceassistant-firmware/assets/106241042/c5efc79b-731b-4fd9-a4db-b2bf3faaf6fe)


7. Once the installation is complete, navigate to `{project_directory/board}`.

8. Copy the files `esp32_s3_korvo1`, `CMakeLists.txt`, and `Kconfig.projbuild`.

9. Paste them in the following directory:
   `{esp-adf-installation-path}\esp-adf\components\audio_board`

10. Select "Replace the files in the destination."

11. Open Git Bash in the ESP-IDF directory and run the following command. Choose the patch according to your ESP-IDF version:
    ```bash
    git apply $ADF_PATH/idf_patches/idf_v4.4_freertos.patch
    ```

12. Delete the `build` and `.vscode` folders from the projects directory to ensure VS-Code finds the ESP-ADF path.

13. Select `esp32s3` as the target.

14. Replace the contents of `sdkconfig` with `sdkconfig.defaults.esp32s3`.

15. Replace WiFi credentials in `{project_directory}/devices/wifi/wifi.h`.

## Additional Notes
- Ensure that your ESP-IDF version matches the patch applied in step 11.
