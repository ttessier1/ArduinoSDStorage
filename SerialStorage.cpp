#include <iostream>
#include <Windows.h>
#include <ktmw32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <winioctl.h>
#include <dbt.h>
#include <bcrypt.h>
#include <msports.h>
#include <ntstatus.h>

#include <string>
#include <sstream>
#include <vector>

using namespace std;

const char* GetBitToLetter(DWORD unitMask);
DWORD EventThread(LPVOID lpData);
LRESULT CALLBACK HiddenWindowProc(HWND unnamedParam1,UINT unnamedParam2,WPARAM unnamedParam3,LPARAM unnamedParam4);

vector<string> GetComPorts();

BOOL DoPing(HANDLE hComm);
BOOL DoListFiles(HANDLE hComm);
BOOL DoDownloadFiles(HANDLE hComm, char * filename, uint64_t fileSize, uint8_t* hash, uint32_t hash_size);
BOOL GetFileSize(HANDLE hComm, char* pszFileName, uint64_t* fileSize);
BOOL GetFileHash(HANDLE hComm, const char* pszFileName, uint8_t* hash, uint32_t hash_size);
BOOL ValidateFileHash(const char * pszFileName, uint8_t * hash, uint32_t hash_size);
BOOL FileExists(const char* pszFileName);
int strlength(const char * message);
std::string string_format(const char * format, ...);

#define CLASS_NAME L"WindoWClassInvisible"

#define BUFFER_SIZE 1024
#define ERROR_BUFFER_SIZE 1024
BOOL bThreadContinue = FALSE;

char error_buffer[ERROR_BUFFER_SIZE];
const char* driveLetter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char* hexChars = "0123456789ABCDEF";
HWND hWndThread = NULL;
int main()
{
    DWORD result = 0;
    DWORD index = 0;
    HANDLE hComm = INVALID_HANDLE_VALUE;
    BOOL bStatus = FALSE;
    BOOL bContinue = FALSE;
    BOOL bInitialized = FALSE;
    int32_t result_value=0;
    DCB dcbSerialParams = { 0 };
    COMMTIMEOUTS commTimeouts = { 0 };
    //char serialBuffer[64];
    uint32_t bytesWritten = 0;
    uint32_t dwEventMask = 0;
    uint32_t noBytesRead = 0;
    uint32_t bytesRead = 0;
    uint32_t zeroBytesRead = 0;
    uint32_t bufferPosition = 0;
    uint64_t fileSize = 0;

    char* buffer = NULL;
    unsigned char loop = 0;
    wchar_t pszPortName[10] = { 0 };
    char pszFileName[MAX_PATH];
    char charBuffer = 0x0;
    char lastChar = 0x00;
    DWORD dwThreadId;
    HANDLE hThread = NULL;
    int optionNo;
    wchar_t PortNo[20] = { 0 };
    vector<string> comPorts;
    getchar();
    hThread = CreateThread(NULL, 0, EventThread, NULL, CREATE_SUSPENDED, &dwThreadId);
    if(hThread==INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to create the thread\r\n");
        return -1;
    }
    if(hThread!=0)
    {
        bThreadContinue = TRUE;
        ResumeThread(hThread);
    }

    buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    if (!buffer)
    {
        fprintf_s(stderr, "Failed to allocate memory, Aborting\r\n");
        return -1;
    }

    comPorts = GetComPorts();
    fprintf(stdout,"ComPorts: %lld\r\n", comPorts.size());
    for ( index = 0 ; index< comPorts.size(); index++)
    {
        fprintf(stdout, "Com Port: %s\r\n",comPorts.at(index).c_str());
    }

    fprintf_s(stdout, "Enter the com port: \r\n");
    wscanf_s(L"%s", pszPortName, (unsigned char)_countof(pszPortName));
    swprintf(PortNo, 20, L"\\\\.\\%s", pszPortName);

    hComm = CreateFile(PortNo, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hComm == INVALID_HANDLE_VALUE)
    {
        fprintf_s(stdout, "Failed to open the com port:%d\r\n",GetLastError() );
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    bStatus = GetCommState(hComm,&dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to get the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }

    //dcbSerialParams.BaudRate = CBR_115200;
    //dcbSerialParams.ByteSize = 8;
    //dcbSerialParams.StopBits = ONESTOPBIT;
    //dcbSerialParams.Parity = NOPARITY;

    //bStatus = SetCommState(hComm,&dcbSerialParams);
    //if (bStatus == FALSE)
   //{
   //     fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
   //     CloseHandle(hComm);
   //     if (buffer)
   //     {
   //         free(buffer);
   //     }
   //     return -1;
   // }

    commTimeouts.ReadIntervalTimeout = 50;
    commTimeouts.ReadTotalTimeoutConstant = 50;
    commTimeouts.ReadTotalTimeoutMultiplier = 10;
    commTimeouts.WriteTotalTimeoutConstant = 50;
    commTimeouts.WriteTotalTimeoutMultiplier= 10;

    if (!SetCommTimeouts(hComm,&commTimeouts))
    {
        fprintf_s(stdout, "Failed to set the comm timeouts: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    /* dcbSerialParams.BaudRate = CBR_1200;
    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    bStatus = EscapeCommFunction(hComm, SETRTS);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to escape the com function: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    bStatus = EscapeCommFunction(hComm, CLRDTR);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to escape the com function: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }

    dcbSerialParams.XonChar = 200;
    dcbSerialParams.XoffChar = 114;
    dcbSerialParams.EofChar = 192;
    dcbSerialParams.ErrorChar = 0x00;
    dcbSerialParams.EvtChar= 0x00;

    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }

    dcbSerialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
    dcbSerialParams.XoffLim = 0;
    dcbSerialParams.XonLim = 0;
    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }    
    
    Sleep(1000);*/
    
    dcbSerialParams.BaudRate = 2000000;
    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    bStatus = EscapeCommFunction(hComm, SETRTS);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to escape the com function: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    bStatus = EscapeCommFunction(hComm, SETDTR);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to escape the com function: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    
   

    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    dcbSerialParams.XonChar = 17;
    dcbSerialParams.XoffChar = 19;
    dcbSerialParams.EofChar = 64;
    dcbSerialParams.ErrorChar = 0x00;
    dcbSerialParams.EvtChar = 0x00;
    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    dcbSerialParams.fRtsControl = 0;
    dcbSerialParams.fDtrControl = DTR_CONTROL_HANDSHAKE;
    dcbSerialParams.XoffLim = 512;
    dcbSerialParams.XonLim = 2048;

    bStatus = SetCommState(hComm, &dcbSerialParams);
    if (bStatus == FALSE)
    {
        fprintf_s(stdout, "Failed to set the comm parameters: %d\r\n", GetLastError());
        CloseHandle(hComm);
        if (buffer)
        {
            free(buffer);
        }
        return -1;
    }
    Sleep(500);

    bContinue = TRUE;
    while (!bInitialized && bContinue)
    {
        if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), (LPDWORD) & bytesRead, NULL))
        {
            if (bytesRead == sizeof(charBuffer))
            {
                zeroBytesRead = 0;
                buffer[bufferPosition] = charBuffer;
                buffer[bufferPosition + 1] = '\0';
                bufferPosition++;
                if (bufferPosition >= BUFFER_SIZE)
                {
                    bContinue = FALSE;
                    fprintf(stderr, "Buffer Overflow for inbound initialization buffer - Aborting\r\n");
                }
                else
                {
                    if (lastChar == '\r' && charBuffer == '\n')
                    {
                        if (memcmp(buffer, "InitSD...OK\r\n", strlen("InitSD...OK\r\n")) == 0)
                        {
                            fprintf_s(stderr, "Initialize SD Card Succeeded: %s\r\n", buffer);
                            bContinue = FALSE;
                            bInitialized = TRUE;
                        }
                        else if (memcmp(buffer, "InitSD...FAIL\r\n", strlen("InitSD...FAIL\r\n")) == 0)
                        {
                            fprintf_s(stderr, "Initialize SD Card Failed: %s\r\n", buffer);
                            bContinue = FALSE;
                        }
                        else
                        {
                            fprintf_s(stderr, "Response not understood: %s\r\n", buffer);
                            bContinue = FALSE;
                        }
                    }

                }
            }
            else
            {
                zeroBytesRead++;
                if (zeroBytesRead > 3)
                {
                    fprintf_s(stderr, "Read too many 0 byte lengths\r\n");
                    bContinue = FALSE;
                }
            }
            lastChar = charBuffer;
        }
        else
        {
            fprintf_s(stderr, "Failed to read from com port: %d\r\n",GetLastError());
            bContinue = FALSE;
        }
    }
    if (!bInitialized)
    {
        if (DoPing(hComm))
        {
            bInitialized = TRUE;
        }
    }
    if (bInitialized)
    {

        DoPing(hComm);

        bContinue = TRUE;
        while (bContinue)
        {
            fprintf_s(stdout, "Select a number:\r\n\t1) list files\r\n\t2) Change directory\r\n\t3) Delete File\r\n\t4) Create File\r\n\t5) Download File\r\n\t6) Get File Size\r\n\t7) Get File Hash\r\n\t8) Exit \r\n");
            if (wscanf_s(L"%d", &optionNo) == 1)
            {
                // We hopefulle successfully got a number
                fprintf_s(stdout, "You chose: %d\r\n", optionNo);
                switch (optionNo)
                {
                case 1: // List Files
                    if (DoPing(hComm))
                    {
                        if (DoListFiles(hComm))
                        {

                        }
                        else
                        {
                            // Failed to list files
                        }
                    }
                    else
                    {
                        // Failed to ping before actual command
                    }
                    break;
                case 2: // Change Directories

                    break;
                case 3: // Delete File

                    break;
                case 4: // Create/Upload File

                    break;
                case 5: // Download File
                    fprintf_s(stdout, "Enter a file to download\r\n");
                    result_value = scanf_s("%s", pszFileName, MAX_PATH);
                    if (result_value == 1)
                    {
                        if (DoPing(hComm))
                        {
                            if (GetFileSize(hComm, pszFileName, &fileSize))
                            {
                                if (GetFileHash(hComm, pszFileName, (uint8_t*)buffer, BUFFER_SIZE))
                                {
                                    if (DoDownloadFiles(hComm, pszFileName, fileSize, (uint8_t*)buffer, BUFFER_SIZE))
                                    {
                                        fprintf(stdout, "Downloaded and Verified file\r\n");
                                    }
                                    else
                                    {
                                        fprintf(stderr, "Failed to Download and Verified file\r\n");
                                        // Failed to list files
                                    }
                                }
                                else
                                {
                                    fprintf_s(stderr, "Failed to get file hash\r\n");
                                }
                            }
                            else
                            {
                                fprintf_s(stderr, "Failed to get file size\r\n");
                            }
                        }
                        else
                        {
                            // Failed to ping before actual command
                            fprintf_s(stderr, "Failed to pre ping storage\r\n");
                        }
                    }
                    else
                    {
                        strerror_s(error_buffer, ERROR_BUFFER_SIZE, errno);
                        fprintf_s(stderr, "Failed to read the input text: %s\r\n", error_buffer);
                    }
                    break;
                case 6: // Get File Size
                    fprintf_s(stdout, "Enter a file to download\r\n");
                    result_value = scanf_s("%s", pszFileName, MAX_PATH);
                    if (result_value == 1)
                    {
                        if (DoPing(hComm))
                        {
                            if (GetFileSize(hComm, pszFileName, &fileSize))
                            {
                                fprintf(stdout, "File Size: %s %lld\r\n", pszFileName, fileSize);
                            }
                            else
                            {
                                fprintf_s(stderr, "Failed to get file size\r\n");
                            }
                        }
                        else
                        {
                            fprintf_s(stderr, "Failed to pre ping storage\r\n");
                        }
                    }
                    else
                    {
                        strerror_s(error_buffer, ERROR_BUFFER_SIZE, errno);
                        fprintf_s(stderr, "Failed to read the input text: %s\r\n", error_buffer);
                    }
                    break;
                case 7: // Get File Hash
                    fprintf_s(stdout, "Enter a file to download\r\n");
                    result_value = scanf_s("%s", pszFileName, MAX_PATH);
                    if (result_value == 1)
                    {
                        if (DoPing(hComm))
                        {
                            if (GetFileHash(hComm, pszFileName, (uint8_t*)buffer, BUFFER_SIZE))
                            {
                                fprintf_s(stderr, "File hash:%s\r\n", buffer);
                            }
                            else
                            {
                                fprintf_s(stderr, "Failed to get file hash\r\n");
                            }
                        }
                        else
                        {
                            fprintf_s(stderr, "Failed to pre ping storage\r\n");
                        }
                    }
                    else
                    {
                        strerror_s(error_buffer, ERROR_BUFFER_SIZE, errno);
                        fprintf_s(stderr, "Failed to read the input text: %s\r\n", error_buffer);
                    }
                    break;
                case 8:
                    bContinue = FALSE;
                    break;
                default:
                    // Unknown Options
                    fprintf_s(stderr, "Unknown Option: %d\r\n", optionNo);
                }
            }
            else
            {
                fprintf_s(stderr, "Failed to read the option\r\n");
                while (getchar() != '\n')
                {

                }
            }

        }
    }
    CloseHandle(hComm);
    if (buffer)
    {
        free(buffer);
    }
    if (hThread != 0)
    {
        bThreadContinue = FALSE;
        SendMessage(hWndThread, WM_CLOSE, 0, 0);
        while (true)
        {

            result = WaitForSingleObject(hThread, 1000);
            if (result == WAIT_ABANDONED)
            {
                fprintf(stderr, "Wait Abandoned for thread\r\n");
                break;
            }
            else if (result == WAIT_TIMEOUT)
            {
                if (zeroBytesRead > 10)
                {
                    fprintf(stderr, "Timeout waiting for Thread ABORT!!!\r\n");
                    TerminateThread(hThread,0);
                    break;
                }
                fprintf(stderr, "Timeout waiting for thread to complete\r\n");
                bThreadContinue = FALSE;
                SendMessage(hWndThread, WM_CLOSE, 0, 0);
                zeroBytesRead++;

            }
            else if (result == WAIT_OBJECT_0)
            {
                fprintf(stderr, "Thread Completed\r\n");
                break;
            }
            else if (result == WAIT_FAILED)
            {
                fprintf(stderr, "Wait Failed for thread\r\n");
                break;
            }
        }
    }
    return 0;
}



const char* GetBitToLetter(DWORD unitMask)
{
    DWORD index = 0;
    for (index = 0; index < 26; index++)
    {
        if ((unitMask & (0x1 << index)) == (0x1 << index))
        {
            return &driveLetter[index];
        }
    }
    return "";
}

DWORD EventThread(LPVOID lpData)
{
    MSG msg;
    BOOL bRet;
    
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASS wndClass;
    wndClass.hInstance = hInstance;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpfnWndProc = HiddenWindowProc;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = CLASS_NAME;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    ATOM classAtom = RegisterClass(&wndClass);
    if(classAtom==NULL)
    {
        MessageBox(HWND_DESKTOP,L"Failed to register the class",L"Error",MB_OK);
        return -1;
    }
    HWND hWnd = CreateWindow(CLASS_NAME, L"Hidden Window", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (hWnd == NULL)
    {
        MessageBox(HWND_DESKTOP, L"Failed to create the window", L"Error", MB_OK);
        return -1;
    }
    hWndThread = hWnd;
    ShowWindow(hWnd, SW_HIDE);
    bThreadContinue = TRUE;
    while ((bRet = GetMessage(&msg, NULL, WM_DEVICECHANGE, WM_DEVICECHANGE)) != 0 && bThreadContinue)
    {
        if (bRet == -1)
        {
            fprintf(stderr, "Error in Getmessage - Aborting: %d\r\n",GetLastError());
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    fprintf(stdout,"Exiting Thread\r\n");
    return 0;
}
LRESULT CALLBACK HiddenWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PDEV_BROADCAST_HDR  lpDevBroadcast;
    PDEV_BROADCAST_DEVICEINTERFACE lpDevInt;
    PDEV_BROADCAST_OEM lpOem;
    PDEV_BROADCAST_PORT lpPortInfo;
    PDEV_BROADCAST_VOLUME lpVolume;
    _DEV_BROADCAST_USERDEFINED * lpUserDefined;
    switch (uMsg)
    {
    case WM_USER:
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        break;
    case WM_CREATE:
        return 0L;
    case WM_DEVICECHANGE:
        switch (wParam)
        {
        case DBT_DEVNODES_CHANGED:
            fprintf(stdout, "Device Nodes Changed\r\n");
            break;
        case DBT_QUERYCHANGECONFIG:
            fprintf(stdout, "Device Dock/Undock Requested\r\n");
            return TRUE;
            break;
        case DBT_CONFIGCHANGED:
            fprintf(stdout, "Device Dock/Undock\r\n");
            break;
        case DBT_CONFIGCHANGECANCELED:
            fprintf(stdout, "Device Change Cancelled\r\n");
            break;
        case DBT_DEVICEQUERYREMOVE:
            fprintf(stdout, "Device Query Remove\r\n");
            // Note LPARAM is Device Broadcast Header
            return TRUE;
            break;
        case DBT_DEVICEQUERYREMOVEFAILED:
            fprintf(stdout, "Device Query Remove Cancelled\r\n");
            // Note LPARAM is Device Broadcast Header
            break;
        case DBT_DEVICEREMOVEPENDING:
            fprintf(stdout, "Device Query Remove Pending\r\n");
            // Note LPARAM is Device Broadcast Header
            break;
        
        case DBT_DEVICEARRIVAL:
            lpDevBroadcast = (PDEV_BROADCAST_HDR)(lParam);
            if (lpDevBroadcast != NULL)
            {
                switch (lpDevBroadcast->dbch_devicetype)
                {
                case DBT_DEVTYP_DEVICEINTERFACE:
                    lpDevInt = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                    fprintf(stdout, "Name: %S Guid : {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\r\n",
                        lpDevInt->dbcc_name,
                        lpDevInt->dbcc_classguid.Data1,
                        lpDevInt->dbcc_classguid.Data2,
                        lpDevInt->dbcc_classguid.Data3,
                        lpDevInt->dbcc_classguid.Data4[0],
                        lpDevInt->dbcc_classguid.Data4[1],
                        lpDevInt->dbcc_classguid.Data4[2],
                        lpDevInt->dbcc_classguid.Data4[3],
                        lpDevInt->dbcc_classguid.Data4[4],
                        lpDevInt->dbcc_classguid.Data4[5],
                        lpDevInt->dbcc_classguid.Data4[6],
                        lpDevInt->dbcc_classguid.Data4[7]
                    );
                    break;
                    /*case DBT_DEVTYP_HANDLE:
                    Do Not Support because we are not receiving DBT_CUSTOM
                        break;*/
                case DBT_DEVTYP_OEM:
                    lpOem = (PDEV_BROADCAST_OEM)lParam;
                    fprintf(stdout, "Identifier:%d\r\n", lpOem->dbco_identifier);
                    break;
                case DBT_DEVTYP_PORT:
                    lpPortInfo = (PDEV_BROADCAST_PORT)lParam;
                    fprintf(stdout, "Device Arrival Name:%S\r\n", lpPortInfo->dbcp_name);
                    break;
                case DBT_DEVTYP_VOLUME:
                    lpVolume = (PDEV_BROADCAST_VOLUME)lParam;
                    if (lpVolume->dbcv_flags == DBTF_MEDIA)
                    {
                        fprintf(stdout, "Volume -Media:%x %*.s\r\n", lpVolume->dbcv_unitmask, 1, GetBitToLetter(lpVolume->dbcv_unitmask));
                    }
                    else if (lpVolume->dbcv_flags == DBTF_NET)
                    {
                        fprintf(stdout, "Volume -Net: %x %*.s\r\n", lpVolume->dbcv_unitmask, 1, GetBitToLetter(lpVolume->dbcv_unitmask));
                    }
                    else
                    {
                        fprintf(stdout, "Volume -Unknown:%x %*.s\r\n", lpVolume->dbcv_unitmask, 1, GetBitToLetter(lpVolume->dbcv_unitmask));
                    }
                    break;

                }
            }
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            lpDevBroadcast = (DEV_BROADCAST_HDR*)(lParam);
            switch (lpDevBroadcast->dbch_devicetype)
            {
            case DBT_DEVTYP_DEVICEINTERFACE:
                lpDevInt = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                fprintf(stdout, "Removed Name: %S Guid : {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\r\n",
                    lpDevInt->dbcc_name,
                    lpDevInt->dbcc_classguid.Data1,
                    lpDevInt->dbcc_classguid.Data2,
                    lpDevInt->dbcc_classguid.Data3,
                    lpDevInt->dbcc_classguid.Data4[0],
                    lpDevInt->dbcc_classguid.Data4[1],
                    lpDevInt->dbcc_classguid.Data4[2],
                    lpDevInt->dbcc_classguid.Data4[3],
                    lpDevInt->dbcc_classguid.Data4[4],
                    lpDevInt->dbcc_classguid.Data4[5],
                    lpDevInt->dbcc_classguid.Data4[6],
                    lpDevInt->dbcc_classguid.Data4[7]
                );
                break;
                /*case DBT_DEVTYP_HANDLE:
                Do Not Support because we are not receiving DBT_CUSTOM
                    break;*/
            case DBT_DEVTYP_OEM:
                lpOem = (PDEV_BROADCAST_OEM)lParam;
                fprintf(stdout, "Removed Identifier:%d\r\n", lpOem->dbco_identifier);
                break;
            case DBT_DEVTYP_PORT:
                lpPortInfo = (PDEV_BROADCAST_PORT)lParam;
                fprintf(stdout, "Removed Name:%S\r\n", lpPortInfo->dbcp_name);
                break;
            case DBT_DEVTYP_VOLUME:
                lpVolume = (PDEV_BROADCAST_VOLUME)lParam;
                if (lpVolume->dbcv_flags == DBTF_MEDIA)
                {
                    fprintf(stdout, "Removed Volume -Media:%x %*.s\r\n", lpVolume->dbcv_unitmask, 1, GetBitToLetter(lpVolume->dbcv_unitmask));
                }
                else if (lpVolume->dbcv_flags == DBTF_NET)
                {
                    fprintf(stdout, "Removed Volume -Net: %x %*.s\r\n", lpVolume->dbcv_unitmask, 1, GetBitToLetter(lpVolume->dbcv_unitmask));
                }
                else
                {
                    fprintf(stdout, "Removed Volume -Unknown:%x %*.s\r\n", lpVolume->dbcv_unitmask, 1, GetBitToLetter(lpVolume->dbcv_unitmask));
                }
                break;

            }
            
            break;
        case DBT_DEVICETYPESPECIFIC:
            fprintf(stdout, "Device Specific Event\r\n");
            // Note LPARAM is Device Broadcast Header
            break;
        case DBT_CUSTOMEVENT:
            fprintf(stdout, "Device Custom Event\r\n");
            // Note LPARAM is Device Broadcast Header
            break;
        case DBT_USERDEFINED:
            
            lpUserDefined = (_DEV_BROADCAST_USERDEFINED * )lParam;
            fprintf(stdout, "Device User Event: %s\r\n", lpUserDefined->dbud_szName);
            
            // Note LPARAM is _DEV_BROADCAST_USERDEFINED 
            break;
        }
        break;
    case WM_DEVMODECHANGE:
        if (lParam != NULL)
        {
            fprintf(stdout, "Device Mode Chage: %S\r\n", (wchar_t*)lParam);
        }
        else
        {
            fprintf(stdout, "Device Mode Chage: (NULL)\r\n");
        }
        break;
    case WM_CLOSE:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0L;
}

#define PING_BUFFER_SIZE 16
BOOL DoPing(HANDLE hComm)
{
    BOOL bContinue = FALSE;
    BOOL returnCode = FALSE;
    uint32_t bytesWritten = 0;
    uint32_t bytesRead = 0;
    uint32_t buffPosition = 0;
    uint32_t zeroBytesRead = 0;
    char charBuffer = 0x0;
    char pingBuffer[PING_BUFFER_SIZE];
    if (hComm!= INVALID_HANDLE_VALUE)
    {
        if (WriteFile(hComm, "0\r\n", strlength("0\r\n"), (LPDWORD) & bytesWritten, NULL) && bytesWritten == strlen("0\r\n"))
        {
            fprintf(stdout, "Wrote: %s %d\r\n ", "0\r\n", bytesWritten);
            bContinue = TRUE;
            while (bContinue)
            {
                if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), (LPDWORD) & bytesRead, NULL))
                {
                    fprintf(stdout, "Char: %c %d\r\n", charBuffer, bytesRead);
                    if (bytesRead == 1)
                    {
                        pingBuffer[buffPosition] = charBuffer;
                        pingBuffer[buffPosition + 1] = '\0';
                        buffPosition++;
                        if (buffPosition >= PING_BUFFER_SIZE)
                        {
                            bContinue = FALSE;
                            fprintf_s(stdout, "Buffer overflow at ping\r\n");
                        }
                        if (strcmp(pingBuffer, "ping\r\n") == 0)
                        {
                            bContinue = FALSE;
                            returnCode = TRUE;
                        }
                    }
                    else
                    {
                        zeroBytesRead++;
                        fprintf(stderr, "Read 0 bytes\r\n");
                        if (zeroBytesRead > 3)
                        {
                            bContinue = FALSE;
                        }
                    }
                }
                else
                {
                    bContinue = FALSE;
                    fprintf_s(stdout, "Failed to read the response: %d for Ping\r\n", GetLastError());
                }
            }
        }
        else
        {
            fprintf_s(stdout, "Failed to write the command: %d for Ping\r\n", GetLastError());
        }
    }
    else
    {
        fprintf_s(stdout, "Invalid Handle sent to Ping\r\n");
    }
    return returnCode;
}

#define LISTFILE_BUFFER_SIZE 1024

BOOL DoListFiles(HANDLE hComm)
{
    BOOL bContinue = FALSE;
    BOOL returnCode = FALSE;
    DWORD bytesWritten = 0;
    DWORD bytesRead = 0;
    DWORD buffPosition = 0;
    char charBuffer=0x00;
    char lastChar=0x00;
    char listBuffer[LISTFILE_BUFFER_SIZE] = { 0 };
    
    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (WriteFile(hComm, "1\r\n", strlength("1\r\n"), &bytesWritten, NULL) && bytesWritten == strlen("1\r\n"))
        {
            fprintf(stdout, "Wrote: %s %d\r\n ", "1\r\n", bytesWritten);
            bContinue = TRUE;
            while (bContinue)
            {
                if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), &bytesRead, NULL) && bytesRead == 1)
                {
                    if (charBuffer != '\r' && charBuffer != '\n')
                    {
                        listBuffer[buffPosition] = charBuffer;
                        listBuffer[buffPosition + 1] = '\0';
                    }
                    if (memcmp(listBuffer, "\0",1) == 0 && charBuffer=='\n' && lastChar=='\r') // end of the list
                    {
                        bContinue = FALSE;
                        returnCode = TRUE;
                    }
                    else
                    {
                        switch (charBuffer)
                        {
                        case '\r':
                            if (lastChar == '\n')
                            {
                                if (strstr(listBuffer, "BAD Read list\r\n"))
                                {
                                    bContinue = FALSE;
                                }
                                else if (strstr(listBuffer, "Unrecognized Command"))
                                {
                                    bContinue = FALSE;
                                }
                                else
                                {
                                    fprintf(stdout, "File: %s\r\n", listBuffer);
                                    buffPosition = 0;
                                    memset(listBuffer, 0, sizeof(listBuffer));
                                }
                            }
                            break;
                        case '\n':
                            if (lastChar == '\r')
                            {
                                if (strstr(listBuffer, "BAD Read list\r\n"))
                                {
                                    bContinue = FALSE;
                                }
                                else if (strstr(listBuffer, "Unrecognized Command"))
                                {
                                    bContinue = FALSE;
                                }
                                else
                                {
                                    fprintf(stdout, "File: %s\r\n", listBuffer);
                                    buffPosition = 0;
                                    memset(listBuffer, 0, sizeof(listBuffer));
                                }
                            }
                            break;

                        default:
                            buffPosition++;
                        }
                    }
                    lastChar = charBuffer;
                    
                    if (buffPosition >= LISTFILE_BUFFER_SIZE)
                    {
                        bContinue = FALSE;
                        fprintf_s(stdout, "Buffer overflow at DoListFiles\r\n");
                    }
                   
                }
                else
                {
                    bContinue = FALSE;
                    fprintf_s(stdout, "Failed to read the response: %d for DoListFiles\r\n", GetLastError());
                }
            }
        }
        else
        {
            fprintf_s(stdout, "Failed to write the command: %d for DoListFiles\r\n", GetLastError());
        }
    }
    else
    {
        fprintf_s(stdout, "Invalid Handle sent to DoListFiles\r\n");
    }
    return returnCode;
}

BOOL DoDownloadFiles(HANDLE hComm, char* pszFileName, uint64_t fileSize, uint8_t * hash, uint32_t hash_size)
{
    BOOL bContinue = FALSE;
    BOOL returnCode = FALSE;
    BOOL isFirstLine = FALSE;
    DWORD bytesWritten = 0;
    DWORD bytesRead = 0;
    DWORD buffPosition = 0;
    char convBuffer[BUFFER_SIZE];
    char qstBuffer[16] = { 0 };
    uint16_t byteCount = 0;
    uint16_t index = 0;
    uint64_t expectedSize = fileSize * 2;
    uint64_t totalBytes = 0;
    int result_value = 0;
    char* buffer = NULL;
    DWORD bufferLength = 0;
    DWORD zeroByteCount = 0;
    char charBuffer = 0x00;
    char lastChar = 0x00;
    HANDLE hTransactionHandle = INVALID_HANDLE_VALUE;;
    HANDLE hFileHandle = INVALID_HANDLE_VALUE;
    //char charBuffer;
    //char pingBuffer[PING_BUFFER_SIZE];
    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (pszFileName != NULL)
        {
            if (fileSize >= 0 && hash != NULL && hash_size >= 65)// make sure we can handle a large hash buffer
            {
                bufferLength = strlength(pszFileName) + strlength("5\r\n");
                buffer = (char*)malloc(bufferLength + 1);
                if (buffer != NULL)
                {
                    memset(buffer, 0, bufferLength + 1);
                    strcpy_s(buffer, bufferLength + 1, "5");
                    strcat_s(buffer, bufferLength + 1, pszFileName);
                    strcat_s(buffer, bufferLength + 1, "\r\n");
                    //hTransactionHandle= CreateTransaction(NULL,0, TRANSACTION_DO_NOT_PROMOTE,0,0,3600,(LPWSTR)L"Download File");
                    //if (hTransactionHandle != INVALID_HANDLE_VALUE)
                    //{
                        // Create folder hierarchy
                        if (strstr(pszFileName, ".\\") == pszFileName)
                        {
                            pszFileName += 2;
                        }
                        if (strchr(pszFileName, '\\'))
                        {
                            //CreateFolderChainTransacted(hTransactionHandle,pszFileName);
                        }
                        // Open file in hierarchy
                        if (FileExists(pszFileName))
                        {
                            while (true)
                            {
                                fprintf(stdout, "File already exists, do you want to overwrite(y/n)?\r\n");
                                result_value = scanf_s("%s", qstBuffer, 16);
                                if (result_value == 1)
                                {
                                    if (_strcmpi(qstBuffer, "y") == 0)
                                    {
                                        break;
                                    }
                                    else if (_strcmpi(qstBuffer, "n") == 0)
                                    {
                                        fprintf(stdout, "User Cancelled\r\n");
                                        if (buffer!=NULL)
                                        {
                                            free(buffer);
                                        }
                                        return FALSE;
                                    }
                                    else
                                    {
                                        fprintf(stderr, "Response Not Understood\r\n");
                                    }
                                    break;
                                }
                                else
                                {
                                    fprintf(stderr, "Invalid Response\r\n");
                                }
                            }
                        }
                        hFileHandle = CreateFileA(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
                        if (hFileHandle != INVALID_HANDLE_VALUE)
                        {
                            SetEndOfFile(hFileHandle);
                            if (WriteFile(hComm, buffer, bufferLength, &bytesWritten, NULL) && bytesWritten == bufferLength)
                            {
                                fprintf(stdout, "Wrote: %s %d\r\n ", buffer, bytesWritten);
                                bContinue = TRUE;
                                isFirstLine = TRUE;
                                while (bContinue)
                                {
                                    SetLastError(0);
                                    if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), &bytesRead, NULL) && bytesRead == 1)
                                    {
                                        if (charBuffer == '\n' && lastChar == '\r')
                                        {
                                            if (isFirstLine)
                                            {
                                                if (_strcmpi(convBuffer, "File Follows") == 0)
                                                {
                                                    isFirstLine = FALSE;
                                                    byteCount = 0;
                                                    convBuffer[0] = '\0';
                                                }
                                                else
                                                {
                                                    fprintf(stderr, "Failed to download File: %s\r\n", convBuffer);
                                                    while (true)
                                                    {
                                                        if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), &bytesRead, NULL) && bytesRead == 1)
                                                        {
                                                            // Read until timeout
                                                        }
                                                        else
                                                        {
                                                            bContinue = FALSE;
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                if (strcmp(convBuffer, "") == 0)
                                                {
                                                    // End of file
                                                    CloseHandle(hFileHandle);
                                                    hFileHandle = INVALID_HANDLE_VALUE;
                                                    if (ValidateFileHash(pszFileName, hash, hash_size))
                                                    {
                                                        bContinue = FALSE;
                                                        returnCode = TRUE;
                                                        fprintf(stdout, "Validated the hash of the local file\r\n");
                                                    }
                                                    else
                                                    {
                                                        bContinue = FALSE;
                                                        returnCode = FALSE;
                                                        fprintf(stderr, "Failed to validate the hash of the local file\r\n");
                                                    }
                                                }
                                                else
                                                {
                                                    if (byteCount % 2 == 0 && byteCount <= min(expectedSize,128))
                                                    {
                                                        totalBytes += byteCount;
                                                        expectedSize -= byteCount;
                                                        for (int i = 0; i < byteCount;)
                                                        {
                                                            if (convBuffer[i] >= '0' && convBuffer[i] <= '9')
                                                            {
                                                                charBuffer = ((convBuffer[i]) - 0x30) << 4;
                                                            }
                                                            else if (convBuffer[i] >= 'A' && convBuffer[i] <= 'F')
                                                            {
                                                                charBuffer = ((convBuffer[i]) - 0x37) << 4;
                                                            }
                                                            else if (convBuffer[i] >= 'a' && convBuffer[i] <= 'f')
                                                            {
                                                                charBuffer = ((convBuffer[i]) - 0x57) << 4;
                                                            }
                                                            i++;
                                                            if (convBuffer[i] >= '0' && convBuffer[i] <= '9')
                                                            {
                                                                charBuffer += ((convBuffer[i]) - 0x30);
                                                            }
                                                            else if (convBuffer[i] >= 'A' && convBuffer[i] <= 'F')
                                                            {
                                                                charBuffer += ((convBuffer[i]) - 0x37);
                                                            }
                                                            else if (convBuffer[i] >= 'a' && convBuffer[i] <= 'f')
                                                            {
                                                                charBuffer += ((convBuffer[i]) - 0x57);
                                                            }
                                                            i++;
                                                            if (WriteFile(hFileHandle, &charBuffer, sizeof(charBuffer), &bytesWritten, NULL) && bytesWritten == sizeof(charBuffer))
                                                            {

                                                            }
                                                            else
                                                            {
                                                                fprintf(stderr, "Failed to write the file\r\n");
                                                                bContinue = FALSE;
                                                            }
                                                        }
                                                        byteCount = 0;
                                                        convBuffer[0] = '\0';
                                                    }
                                                    else
                                                    {
                                                        byteCount = 0;
                                                        bContinue = FALSE;
                                                    }

                                                }
                                            }
                                        }
                                        if (charBuffer != '\n' && charBuffer != '\r')
                                        {
                                            convBuffer[byteCount] = charBuffer;
                                            convBuffer[byteCount + 1] = '\0';
                                            byteCount++;
                                        }
                                        lastChar = charBuffer;
                                    }
                                    else
                                    {
                                        if (GetLastError() == 0 && bytesRead == 0)
                                        {
                                            zeroByteCount++;
                                            if (zeroByteCount > 3)
                                            {
                                                fprintf(stderr, "Failed to read: %d\r\n", GetLastError());
                                                bContinue = FALSE;
                                            }
                                        }
                                        else
                                        {
                                            fprintf(stderr, "Failed to read: %d\r\n", GetLastError());
                                            bContinue = FALSE;
                                        }
                                    }
                                }
                                if (returnCode)
                                {
                                    if (hFileHandle != INVALID_HANDLE_VALUE)
                                    {
                                        CloseHandle(hFileHandle);
                                        hFileHandle = INVALID_HANDLE_VALUE;
                                    }
                                    /*if (CommitTransaction(hTransactionHandle))
                                    {
                                        fprintf(stdout, "Successfully Committed back Transaction\r\n");
                                    }
                                    else
                                    {
                                        fprintf(stderr, "Failed to Commit Transaction: %d\r\n", GetLastError());
                                    }*/
                                }
                                else
                                {
                                    if (hFileHandle != INVALID_HANDLE_VALUE)
                                    {
                                        CloseHandle(hFileHandle);
                                        hFileHandle = INVALID_HANDLE_VALUE;
                                    }
                                    /*if (RollbackTransaction(hTransactionHandle))
                                    {
                                        fprintf(stdout, "Successfully Rolled back Transaction\r\n");
                                    }
                                    else
                                    {
                                        fprintf(stderr, "Failed Rolled back Transaction: %d\r\n", GetLastError());
                                    }*/
                                }
                                
                            }
                            else
                            {
                                fprintf_s(stdout, "Failed to write the command: %d for DoDownloadFiles\r\n", GetLastError());
                            }
                        }
                        else
                        {
                            fprintf_s(stderr, "Failed to create file : %d\r\n", GetLastError());
                        }
                        //CloseHandle(hTransactionHandle);
                    //}
                    //else
                    //{
                    //    fprintf_s(stderr, "Failed to create transaction\r\n");
                    //}
                    free(buffer);
                }
                else
                {
                    fprintf_s(stderr, "Failed memory allocation in DoDownloadFiles\r\n");
                }
            }
            else
            {
                fprintf_s(stderr, "Invalid file size or file hash sent to DoDownloadFiles\r\n");
            }
        }
        else
        {
            fprintf_s(stderr, "Invalid filename:(NULL) sent to DoDownloadFiles\r\n");
        }
    }
    else
    {
        fprintf_s(stderr, "Invalid Handle sent to DoDownloadFiles\r\n");
    }
    return returnCode;
}

BOOL GetFileSize(HANDLE hComm, char* pszFileName, uint64_t * fileSize)
{
    BOOL returnCode = FALSE;
    BOOL bContinue = FALSE;
    char* buffer = NULL;
    char charBuffer = 0x00;
    char lastChar = 0x00;
    char convBuffer[BUFFER_SIZE];
    uint32_t bytesWritten = 0;
    uint32_t bytesRead = 0;
    uint32_t bufferLength = 0;
    uint32_t zeroByteCount = 0;
    uint16_t byteCount = 0;
    uint16_t index = 0 ;
    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (pszFileName != NULL)
        {
            if (fileSize != NULL)
            {
                bufferLength = strlength(pszFileName) + strlength("6\r\n");
                
                buffer = (char*)malloc(bufferLength + 1);
                if (buffer != NULL)
                {
                    memset(buffer, 0, bufferLength+1);
                    strcpy_s(buffer, bufferLength + 1, "6");
                    strcat_s(buffer, bufferLength + 1, pszFileName);
                    strcat_s(buffer, bufferLength + 1, "\r\n");

                    if (WriteFile(hComm, buffer, bufferLength, (LPDWORD) & bytesWritten, NULL) && bytesWritten == bufferLength)
                    {
                        fprintf(stdout, "Wrote: %s %d\r\n ", buffer, bytesWritten);
                        bContinue = TRUE;
                        *fileSize = 0;
                        while (bContinue)
                        {
                            if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), (LPDWORD) & bytesRead, NULL) && bytesRead == sizeof(charBuffer))
                            {
                                
                                if (charBuffer == '\n' && lastChar == '\r' )
                                {
                                    for (index = 0; index < byteCount; index++)
                                    {
                                        if (convBuffer[index] >= '0' && convBuffer[index] <= '9')
                                        {
                                            *fileSize += (uint64_t)(convBuffer[index]  - 0x30)<< ((byteCount - index-1) * 4);
                                        }
                                        else if (convBuffer[index] >= 'A' && convBuffer[index] <= 'F')
                                        {
                                            *fileSize += (uint64_t)(convBuffer[index] - 0x37) << ((byteCount - index-1) * 4);
                                        }
                                        else if (convBuffer[index] >= 'a' && convBuffer[index] <= 'f')
                                        {
                                            *fileSize += (uint64_t)(convBuffer[index] - 0x57) << ((byteCount - index-1) * 4);
                                        }
                                        else
                                        {
                                            fprintf(stderr, "Found invalid characterin hex conversion:%s \r\n", convBuffer);
                                            bContinue = FALSE;
                                            returnCode = FALSE;
                                        }
                                        
                                    }
                                    if (bContinue)
                                    {
                                        bContinue = FALSE;
                                        returnCode = TRUE;
                                    }
                                    break;
                                }
                                if (charBuffer != '\n' && charBuffer != '\r')
                                {
                                    convBuffer[byteCount] = charBuffer;
                                    convBuffer[byteCount+1] = '\0';
                                    byteCount++;
                                }
                                
                                if (byteCount >= BUFFER_SIZE)
                                {
                                    fprintf(stderr, "Buffer Overload");
                                    bContinue = false;
                                    break;
                                }
                                lastChar = charBuffer;
                            }
                            else
                            {
                                if (GetLastError() == 0 && bytesRead == 0)
                                {
                                    zeroByteCount++;
                                    if (zeroByteCount > 3)
                                    {
                                        fprintf(stderr,"Failed to read: %d\r\n", GetLastError());
                                        bContinue = FALSE;
                                    }
                                }
                                else
                                {
                                    fprintf(stderr, "Failed to read: %d\r\n", GetLastError());
                                    bContinue = FALSE;
                                }
                            }
                            lastChar = charBuffer;
                        }
                    }
                    else
                    {
                        fprintf_s(stdout, "Failed to write the command: %d for GetFileSize\r\n", GetLastError());
                    }
                    free(buffer);
                }
                else
                {
                    fprintf_s(stderr, "Failed memory allocation in GetFileSize\r\n");
                }
            }
            else
            {
                fprintf_s(stderr, "Invalid FileSize:(NULL) sent to GetFileSize\r\n");
            }
        }
        else
        {
            fprintf_s(stderr, "Invalid FileName:(NULL) sent to GetFileSize\r\n");
        }
    }
    else
    {
        fprintf_s(stderr, "Invalid Handle sent to GetFileSize\r\n");
    }
    return returnCode;
}
BOOL GetFileHash(HANDLE hComm,const char * pszFileName, uint8_t * hash, uint32_t hash_size)
{
    BOOL returnCode = FALSE;
    BOOL bContinue = FALSE;
    errno_t value;
    char* buffer = NULL;
    char charBuffer = 0x00;
    char lastChar = 0x00;
    char convBuffer[BUFFER_SIZE];
    uint32_t bytesWritten = 0;
    uint32_t bytesRead = 0;
    uint32_t bufferLength = 0;
    uint32_t zeroByteCount = 0;
    uint16_t byteCount = 0;
    uint16_t index = 0;
    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (pszFileName != NULL)
        {
            if (hash != NULL && hash_size >= 65)// make sure we can handle a large hash buffer
            {
                bufferLength = strlength(pszFileName) + strlength("7\r\n");

                buffer = (char*)malloc(bufferLength + 1);
                if (buffer != NULL)
                {
                    memset(buffer, 0, bufferLength + 1);
                    strcpy_s(buffer, bufferLength + 1, "7");
                    strcat_s(buffer, bufferLength + 1, pszFileName);
                    strcat_s(buffer, bufferLength + 1, "\r\n");

                    if (WriteFile(hComm, buffer, bufferLength, (LPDWORD)&bytesWritten, NULL) && bytesWritten == bufferLength)
                    {
                        fprintf(stdout, "Wrote: %s %d\r\n ", buffer, bytesWritten);
                        bContinue = TRUE;
                        memset(hash, 0, hash_size);
                        while (bContinue)
                        {
                            if (ReadFile(hComm, &charBuffer, sizeof(charBuffer), (LPDWORD)&bytesRead, NULL) && bytesRead == sizeof(charBuffer))
                            {

                                if (charBuffer == '\n' && lastChar == '\r')
                                {
                                    if (strcmp(convBuffer, ".") != 0)
                                    {
                                        value = strcpy_s((char *)hash, hash_size,convBuffer);
                                        if (value == 0 && bContinue)
                                        {
                                            bContinue = FALSE;
                                            returnCode = TRUE;
                                        }
                                        else
                                        {
                                            strerror_s(error_buffer, ERROR_BUFFER_SIZE, value);
                                        }
                                        break;
                                    }
                                    else
                                    {
                                        byteCount = 0;
                                    }
                                }
                                if (charBuffer != '\n' && charBuffer != '\r')
                                {
                                    convBuffer[byteCount] = charBuffer;
                                    convBuffer[byteCount + 1] = '\0';
                                    byteCount++;
                                }

                                if (byteCount >= BUFFER_SIZE)
                                {
                                    fprintf(stderr, "Buffer Overload");
                                    bContinue = false;
                                    break;
                                }
                                lastChar = charBuffer;
                            }
                            else
                            {
                                if (GetLastError() == 0 && bytesRead == 0)
                                {
                                    zeroByteCount++;
                                    if (zeroByteCount > 3)
                                    {
                                        fprintf(stderr, "");
                                        bContinue = FALSE;
                                    }
                                }
                            }
                            lastChar = charBuffer;
                        }
                    }
                    else
                    {
                        fprintf_s(stdout, "Failed to write the command: %d for GetFileSize\r\n", GetLastError());
                    }
                    free(buffer);
                }
                else
                {
                    fprintf_s(stderr, "Failed memory allocation in GetFileSize\r\n");
                }
            }
            else
            {
                fprintf_s(stderr, "Invalid hash buffer:(NULL) sent to GetFileHash\r\n");
            }
        }
        else
        {
            fprintf_s(stderr, "Invalid FileName:(NULL) sent to GetFileHash\r\n");
        }
    }
    else
    {
        fprintf_s(stderr, "Invalid Handle sent to GetFileHash\r\n");
    }
    return returnCode;
}

int strlength(const char* message)
{
    int length = 0;
    if (message != NULL)
    {
        while (*message != '\0')
        {
            message++;
            length++;
        }
    }
    return length;
}

BOOL ValidateFileHash(const char* pszFileName, uint8_t* hash, uint32_t hash_size)
{
    BOOL bContinue = FALSE;
    BOOL returnCode = FALSE;
    DWORD result = 0;
    ULONG size_required=0;
    BCRYPT_ALG_HANDLE algorithmHandle;
    BCRYPT_HASH_HANDLE hashHandle;
    uint8_t generatedHash[326];
    uint8_t hexGeneratedHash[65];
    DWORD bytesRead = 0;
    uint32_t index = 0;
    unsigned char buffer[BUFFER_SIZE];
    DWORD objectsize = 0;
    HANDLE hFileHandle = INVALID_HANDLE_VALUE;
    result = BCryptOpenAlgorithmProvider(&algorithmHandle, BCRYPT_SHA256_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_HASH_REUSABLE_FLAG);
    if (BCRYPT_SUCCESS(result))
    {
        result = BCryptCreateHash(algorithmHandle, &hashHandle, generatedHash, 326, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
        if (BCRYPT_SUCCESS(result))
        {
            hFileHandle = CreateFileA(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hFileHandle != INVALID_HANDLE_VALUE)
            {
                bContinue = TRUE;
                while (bContinue)
                {
                    if (ReadFile(hFileHandle, buffer, BUFFER_SIZE, &bytesRead, NULL))
                    {
                        if (bytesRead == BUFFER_SIZE)
                        {
                            if (BCRYPT_SUCCESS(BCryptHashData(hashHandle, buffer, BUFFER_SIZE, 0)))
                            {

                            }
                            else
                            {
                                fprintf(stderr, "Failed to perform hash round\r\n");
                                bContinue = FALSE;
                            }
                        }
                        else
                        {
                            if (BCRYPT_SUCCESS(BCryptHashData(hashHandle, buffer, bytesRead, 0)))
                            {
                                if (BCRYPT_SUCCESS(BCryptFinishHash(hashHandle, generatedHash, 32, 0)))
                                {
                                    bContinue = FALSE;
                                    returnCode = TRUE;
                                }
                                else
                                {
                                    bContinue = FALSE;
                                    returnCode = FALSE;
                                    fprintf(stderr, "Failed to perform FINAL hash round\r\n");
                                }
                            }
                            else
                            {
                                fprintf(stderr, "Failed to perform hash round\r\n");
                                bContinue = FALSE;
                            }
                           
                        }
                    }
                    else
                    {
                        bContinue = FALSE;
                        fprintf(stderr, "FAiled to read the file: %d\r\n", GetLastError());
                    }
                }
                CloseHandle(hFileHandle);
                if (returnCode)
                {
                    for (int i = 0; i < 32; i++)
                    {
                        hexGeneratedHash[index] = hexChars[(generatedHash[i] & 0xF0) >> 4];
                        index++;
                        hexGeneratedHash[index] = hexChars[generatedHash[i] & 0xF];
                        index++;
                        hexGeneratedHash[index] = '\0';
                    }
                    if (_strcmpi((char*)hexGeneratedHash, (char*)hash) == 0)
                    {
                        returnCode = TRUE;
                    }
                    else
                    {
                        returnCode = FALSE;
                    }
                }

            }
            else
            {
                
                fprintf(stderr, "Failed to open file: %s for hash verification\r\n", pszFileName);
            }
            BCryptDestroyHash(hashHandle);

        }
        else
        {
            if (STATUS_BUFFER_TOO_SMALL == result)
            {
                result = BCryptGetProperty(algorithmHandle, BCRYPT_OBJECT_LENGTH, (PUCHAR) & objectsize, 4, &size_required, 0);
                if (result == STATUS_BUFFER_TOO_SMALL || result == 0 )
                {
                    fprintf(stderr,"Size: %d", objectsize);
                }
                else
                {
                    fprintf(stderr, "Failed to get object property: %d\r\n", result);
                }
                fprintf(stderr, "Failed to open create sha256 hash: BUFFER TOO SMALL\r\n");
            }
            else
            {
                fprintf(stderr, "Failed to open create sha256 hash: %d\r\n", result);
            }
            
        }
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
            
    }
    else
    {
        fprintf(stderr, "Failed to open a suitable crypto provider for sha256: %d\r\n", result);
    }
    return returnCode;
}

BOOL FileExists(const char* pszFileName)
{
    BOOL returnCode = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    if (pszFileName != NULL)
    {
        hFile = CreateFileA(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            // File Exists
            returnCode = TRUE;
        }
    }
    return returnCode;
}

vector<string> GetComPorts()
{
    HCOMDB comDb;
    vector<string> comPorts;
    DWORD numberOfPorts = 0;
    DWORD index = 0;
    DWORD usedCount = 0;
    BYTE * comPortByteArray=NULL;
    LONG result = ComDBOpen(&comDb);
    if ( result == ERROR_SUCCESS)
    {
        // only check for success and report error if not 
        result = ComDBGetCurrentPortUsage(comDb, NULL, 0, CDB_REPORT_BYTES, &numberOfPorts);
        if ( result == ERROR_SUCCESS)
        {
            // if we have more than one report it
            if (numberOfPorts > 0)
            {
                fprintf(stdout, "There are %d ports available\r\n", numberOfPorts);
                comPortByteArray = (BYTE*)malloc(numberOfPorts);
                if (comPortByteArray != NULL)
                {
                    result = ComDBGetCurrentPortUsage(comDb, comPortByteArray, numberOfPorts, CDB_REPORT_BYTES, &numberOfPorts);
                    if ( result == ERROR_SUCCESS)
                    {
                        for (index = 0; index < numberOfPorts; index++)
                        {
                            if (comPortByteArray[index] != 0)
                            {
                                usedCount++;
                                comPorts.push_back(string_format("Com%d", index + 1));
                            }
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Failed to query the com database with buffer: %d\r\n", result);
                    }

                }
                else
                {
                    fprintf(stderr, "Failed to allocate memory for com database ports\r\n");
                }
            }
            else
            {
                fprintf(stderr, "There do not appear to be any com ports available\r\n");
            }
        }
        else
        {
            fprintf(stderr, "Failed to query the com database: %d\r\n", result);
        }
        ComDBClose(comDb);
    }
    else
    {
        fprintf(stderr, "Failed to open the com database: %d\r\n", result);
    }
    return comPorts;
}

std::string string_format(const char* format, ...)
{
    va_list vl;
    int i;
    stringstream s;

    //  szTypes is the last argument specified; you must access
    //  all others using the variable-argument macros.
    va_start(vl, format);

    // Step through the list.
    for (i = 0; format[i] != '\0'; ++i) {
        union Printable_t {
            int     i;
            float   f;
            double d;
            char    c;
            char* s;
        } Printable;
        if (format[i] == '%')
        {
            i++;
            switch (format[i]) {   // Type to expect.
            case 'i':
            case 'd':
                Printable.i = va_arg(vl, int);
                s << Printable.i;
                break;

            case 'f':
                Printable.d = va_arg(vl, double);
                s << Printable.d;
                break;

            case 'c':
                Printable.c = va_arg(vl, char);
                s << Printable.c;
                break;

            case 's':
                Printable.s = va_arg(vl, char*);
                s << Printable.s;
                break;
            case '%':
                s << "%";
                break;

            default:
                break;
            }
        }
        else
        {
            s << format[i];
        }
    }
    va_end(vl);
    return s.str();
}