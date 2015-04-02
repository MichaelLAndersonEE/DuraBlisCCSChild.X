/* File:   pnet.c  Pnet comm stack.
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F064B
 * Created: 25 Jun 14
 * PNet version 'A' here
 */

#include <xc.h>
#include <ctype.h>      // For screening UID
#include "DuraBlisChild.h"
#include "pnet.h"
#include "serial.h"

#define VERSION_DEFINED_PARMS 4

void respondCalParm(byte ch, byte parmNum, float parmVal);
void respondUIDRequest(byte ch);
void respondNUID(byte ch);
void respondAttention(byte ch);
void respondRHumidity(byte ch);
void respondRelayRequest(byte ch, char relayNo, char onOff);
void respondPowerSwitchRequest(byte ch, char relayNo, char onOff);
void respondStatus(byte ch);
void respondTemperature(byte ch);
void respondSecondaryTemperature(byte ch);

extern byte pnetNodeNumber;
extern byte sysStat;

    // Abbreviated syntax
    // Message              Input        Output         State   Note
    // ----------------------------------------------------------------------
    // Attention node n     AT n        ak n v ss       10
    // Your temperature?    T? n        tk n 095.6      20      atof-able, Fahr
    // Your sec temp?       S? n        sk n 095.6      25
    // Your humidity        H? n        hk n 023.4      30      rel hum, 0 - 100
    // Relay a|b on|off     K n a|b+|-  kk n a|b +|-    40
    // SwiPwr               P n a|b+|-  pk n a|b +|-    50      used for IntFans
    // Your Unique ID?      UID? n      uid n U(10)     60
    // Your new UID is      NUID nU(10) nk n U(10)      70
    // Reset all nodes      RESET                      100
    // Cal consts           CAL m nFFF.F ck m n FFF.F  110

    // Refer to PNet Communication Protocol doc
void childStateMach(char charCh)   // Child emulation
{
    static unsigned int stMach = 0;
    static byte nodeNumber, parmNumber;
    static char relayName, relayAction;
    static float parmVal;
                                    // LineFeeds mean a serial dump
    if ((charCh == CANCEL) || (charCh == LINE_FEED)) stMach = 0;
 
    switch(stMach)
    {
        case 0:     
            if (charCh == 'A') stMach = 10;
            else if (charCh == 'T') stMach = 20;
            else if (charCh == 'S') stMach = 25;
            else if (charCh == 'H') stMach = 30;
            else if (charCh == 'K') stMach = 40;
            else if (charCh == 'P') stMach = 50;
            else if (charCh == 'U') stMach = 60;
            else if (charCh == 'N') stMach = 70;
            else if (charCh == 'R') stMach = 100;
            else if (charCh == 'C') stMach = 110;
            else stMach = 0;
            break;

        case 10:        // Reading A messages
             if (charCh == 'T') stMach = 11;
             else stMach = 0;
             break;

        case 11:
            if (charCh > '0' && charCh < '9')
            {
                nodeNumber = charCh - '0';
                stMach = 12;
            }
            else stMach = 0;
            break;

         case 12:
            if (charCh == CARRIAGE_RETURN)
                respondAttention(nodeNumber);       // respond functions will decode node number and act appropriately
            stMach = 0;     // Terminus
            break;

        case 20:        // Reading T messages
             if (charCh == '?') stMach = 21;
             else stMach = 0;
             break;

        case 21:           
            if (charCh > '0' && charCh < '9')
            {
                nodeNumber = charCh - '0';
                stMach = 22;
            }
            else stMach = 0;
            break;

        case 22:
            if (charCh == CARRIAGE_RETURN)
                respondTemperature(nodeNumber);          
            stMach = 0;     // Terminus
            break;

        case 25:        // Reading S messages
             if (charCh == '?') stMach = 26;
             else stMach = 0;
             break;

        case 26:
            if (charCh > '0' && charCh < '9')
            {
                nodeNumber = charCh - '0';
                stMach = 27;
            }
            else stMach = 0;
            break;

        case 27:
            if (charCh == CARRIAGE_RETURN)
                respondSecondaryTemperature(nodeNumber);
            stMach = 0;     // Terminus
            break;

        case 30:        // Reading H messages
             if (charCh == '?') stMach = 31;
             else stMach = 0;
             break;

        case 31:           
            if (charCh > '0' && charCh < '9')
            {
                nodeNumber = charCh - '0';
                stMach = 32;
            }
            else stMach = 0;
            break;

       case 32:
            if (charCh == CARRIAGE_RETURN)
                respondRHumidity(nodeNumber);           
            stMach = 0;     // Terminus
            break;

        case 40:        // Reading K service requests
            if (charCh > '0' && charCh < '9')
            {
                stMach = 41;
                nodeNumber = charCh - '0';
            }          
            else stMach = 0;
            break;

        case 41:
            if (charCh == 'a' || charCh == 'b')
            {
                stMach = 42;
                relayName = charCh;  
            }
            else stMach = 0;
            break;

        case 42:           
            if (charCh == '+') { relayAction = '+'; stMach = 43; }
            else if (charCh == '-') { relayAction = '-'; stMach = 43; }
            else stMach = 0;     // Terminus
            break;

        case 43:
            if (charCh == CARRIAGE_RETURN)
                respondRelayRequest(nodeNumber, relayName, relayAction);
            stMach = 0;     // Terminus
            break;         

        case 50:        // Reading P service requests
            if (charCh > '0' && charCh < '9')
            {
                stMach = 51;
                nodeNumber = charCh - '0';
            }
            else stMach = 0;
            break;
    
        case 51:
            if (charCh == 'a' || charCh == 'b')
            {
                stMach = 52;
                relayName = charCh;
            }
            else stMach = 0;
            break;

        case 52:
            if (charCh == '+') { relayAction = '+'; stMach = 53; }
            else if (charCh == '-') { relayAction = '-'; stMach = 53; }
            else stMach = 0;     // Terminus
            break;

        case 53:
            if (charCh == CARRIAGE_RETURN)
                respondPowerSwitchRequest(nodeNumber, relayName, relayAction);
            stMach = 0;     // Terminus
            break;

        case 60:
            if (charCh == 'I') stMach = 61;
            else stMach = 0;
            break;

        case 61:
            if (charCh == 'D') stMach = 62;
            else stMach = 0;
            break;

        case 62:
            if (charCh == '?') stMach = 63;
            else stMach = 0;
            break;

        case 63:
            if (charCh > '0' && charCh < '9')
            {
                stMach = 64;
                nodeNumber = charCh - '0';
            }
            else stMach = 0;
            break;

        case 64:
            if (charCh == CARRIAGE_RETURN)
                respondUIDRequest(nodeNumber);
            stMach = 0;     // Terminus
            break;

        case 70:
            if (charCh == 'U') stMach = 71;
            else stMach = 0;
            break;

        case 71:
            if (charCh == 'I') stMach = 72;
            else stMach = 0;
            break;

        case 72:
            if (charCh == 'D') stMach = 73;
            else stMach = 0;
            break;

        case 73:
            if (nodeNumber == (charCh - '0'))
            {
                stMach = 74;
                nodeNumber = charCh - '0';
            }
            else stMach = 0;
            break;

        case 74:
            ioBfr[1] = 0;
            if isalnum(charCh)
            {
                stMach = 75;
                ioBfr[0] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 75:
            ioBfr[2] = 0;
            if isalnum(charCh)
            {
                stMach = 76;
                ioBfr[1] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 76:
            ioBfr[3] = 0;
            if isalnum(charCh)
            {
                stMach = 77;
                ioBfr[2] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 77:
            ioBfr[4] = 0;
            if isalnum(charCh)
            {
                stMach = 78;
                ioBfr[3] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

         case 78:
            ioBfr[5] = 0;
            if isalnum(charCh)
            {
                stMach = 79;
                ioBfr[4] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 79:
            ioBfr[6] = 0;
            if isalnum(charCh)
            {
                stMach = 80;
                ioBfr[5] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 80:
            ioBfr[7] = 0;
            if isalnum(charCh)
            {
                stMach = 81;
                ioBfr[6] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 81:
            ioBfr[9] = 0;
            if isalnum(charCh)
            {
                stMach = 82;
                ioBfr[8] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 82:
            ioBfr[10] = 0;
            if isalnum(charCh)
            {
                stMach = 90;
                ioBfr[9] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 90;
            else stMach = 0;
            break;

        case 90:           
            respondNUID(nodeNumber);
            stMach = 0;     // Terminus
            break;

        case 100:
            if (charCh == 'E') stMach = 81;
            else stMach = 0;
            break;

        case 101:
            if (charCh == 'S') stMach = 82;
            else stMach = 0;
            break;

        case 102:
            if (charCh == 'E') stMach = 83;
            else stMach = 0;
            break;

        case 103:
            if (charCh == 'T')
            {
                SYSKEY = 0xAA996655;
                SYSKEY = 0x556699AA;
                RSWRSTbits.SWRST = 1;
                while(1);       // Seppuku
            }
            stMach = 0;
            break;

       case 110:
            if (charCh == 'A') stMach = 111;
            else stMach = 0;
            break;

       case 111:
            if (charCh == 'L') stMach = 112;
            else stMach = 0;
            break;

        case 112:
            if (charCh > '0' && charCh < '9')
            {
                stMach = 113;
                nodeNumber = charCh - '0';
            }
            else stMach = 0;
            break;

        case 113:
            if (charCh > '0' && charCh < '9')
            {
                stMach = 114;
                parmNumber = charCh - '0';
                if (parmNumber > VERSION_DEFINED_PARMS) stMach = 0;
            }
            else stMach = 0;
            break;

        case 114:
            ioBfr[1] = 0;
            if (isdigit(charCh) || (charCh == '.'))
            {
                stMach = 115;
                ioBfr[0] = charCh;
            }
            else if (charCh == CARRIAGE_RETURN) stMach = 120;
            else stMach = 0;
            break;

        case 120:
            parmVal = atof(ioBfr);
            if (parmVal < 0.5 || parmVal > 2.0) { stMach = 0; return; }
            respondCalParm(nodeNumber, parmNumber, parmVal);
            stMach = 0;     // Terminus
            break;

        default:
            stMach = 0;     // Any syntax error resets
    }
}

    // Cal parm order...
    // double temper1CalFactor = 1.0;
    // double temper2CalFactor = 1.0;
    // double rhumidCalFactor = 1.0;
    // double vRef = 2.992;
    // ---------------------------
void respondCalParm(byte ch, byte parmNum, float parmVal)
{
    extern double temper1CalFactor;
    extern double temper2CalFactor;
    extern double rhumidCalFactor;
    extern double vRef;

    if (ch == pnetNodeNumber)
    {    
        rs485TransmitEna();
        putStr("ck");
        putChar('0' + pnetNodeNumber);
        if (parmNum >= VERSION_DEFINED_PARMS) { putStr("ir\r\n"); }
        else
        {
            switch(parmNum)
            {
                case 0:
                    temper1CalFactor = parmVal;
                    break;
                case 1:
                    temper2CalFactor = parmVal;
                    break;
                case 2:
                    rhumidCalFactor = parmVal;
                    break;
                case 3:
                    vRef = parmVal;
                    break;
            }
            putChar('0' + parmNum);
            sprintf(ioBfr, "%4.03f\r\n", parmVal);
            putStr(ioBfr);
        }
        rs485TransmitDisa();
    }
}

    // ---------------------------
void respondAttention(byte ch)
{   
    extern char pnetVer;
    
    if (ch == pnetNodeNumber)
    {
        rs485TransmitEna();
        putStr("ak");
        putChar('0' + pnetNodeNumber);
        putChar(pnetVer);
        sprintf(ioBfr, "%02X\r\n", sysStat);
        putStr(ioBfr);
        rs485TransmitDisa();
    }
}

    // ---------------------------
void respondNUID(byte ch)
{
    extern char uniqueID[11];

    if (ch == pnetNodeNumber)
    {
        rs485TransmitEna();
        strcpy(uniqueID, ioBfr);       
        putStr("nk");
        putChar('0' + pnetNodeNumber);
        putStr(uniqueID);
        putStr("\r\n");
        rs485TransmitDisa();
    }
}

    // ---------------------------
void respondRHumidity(byte ch)
{
    extern double rhumidNow;

    if (ch == pnetNodeNumber)
    {
        rs485TransmitEna();
        putStr("hk");
        putChar('0' + pnetNodeNumber);
        if (sysStat & ST_HSEN_OKAY)
        {            
            sprintf(ioBfr, "%04.1f\r\n", rhumidNow);
            putStr(ioBfr);
        }
       
        rs485TransmitDisa();
    }
}

    // ------------------------
void respondRelayRequest(byte ch, char relayNo, char onOff)
{   
    if (ch == pnetNodeNumber)
    {     
        rs485TransmitEna();
        putStr("kk");
        putChar('0' + pnetNodeNumber);
        if (relayNo == 'a')
        {
            if (onOff == '+')
            {
                sysStat |= ST_K1_ON;
                RELAY1 = 1;
                putStr("a+\r\n");
            }
            else
            {
                sysStat &= ~ST_K1_ON;
                RELAY1 = 0;
                putStr("a-\r\n");
            }
        }
        else if (relayNo == 'b')
        {
            if (onOff == '+')
            {
                sysStat |= ST_K2_ON;
                RELAY2 = 1;
                putStr("b+\r\n");
            }
            else
            {
                sysStat &= ~ST_K2_ON;
                RELAY2 = 0;
                putStr("b-\r\n");
            }
        }
        rs485TransmitDisa();
    }
}

  // ------------------------
void respondPowerSwitchRequest(byte ch, char relayNo, char onOff)
{
    if (ch == pnetNodeNumber)
    {     
        rs485TransmitEna();
        putStr("pk");
        putChar('0' + pnetNodeNumber);
        if (relayNo == 'a')
        {
            if (onOff == '+')
            {
                sysStat |= ST_SwiPwr1_ON;
                SwiPwr1 = 1;
                putStr("a+\r\n");
            }
            else
            {
                sysStat &= ~ST_SwiPwr1_ON;
                SwiPwr1 = 0;
                putStr("a-\r\n");
            }
        }
        else if (relayNo == 'b')
        {
            if (onOff == '+')
            {
                sysStat |= ST_SwiPwr2_ON;
                SwiPwr2 = 1;
                putStr("b+\r\n");
            }
            else
            {
                sysStat &= ~ST_SwiPwr2_ON;
                SwiPwr2 = 0;
                putStr("b-\r\n");
            }
        }
        rs485TransmitDisa();
    }
}
 
    // ---------------------------
void respondTemperature(byte ch)
{
    extern double temperNowF;

    if (ch == pnetNodeNumber)
    {   
        rs485TransmitEna();
        putStr("tk");
        putChar('0' + pnetNodeNumber);
        if (sysStat & ST_HSEN_OKAY)
        {
            sprintf(ioBfr, "%04.1f", temperNowF);
            putStr(ioBfr);
        }
        else
        {
            putStr("ir");
        }
        putStr("\r\n");
        rs485TransmitDisa();
    }
}

    // ---------------------------
void respondSecondaryTemperature(byte ch)
{
    extern double temperSecondaryF;

    if (ch == pnetNodeNumber)
    {
        rs485TransmitEna();
        putStr("sk");
        putChar('0' + pnetNodeNumber);
        if (sysStat & ST_HSEN_OKAY)
        {
            sprintf(ioBfr, "%04.1f", temperSecondaryF);
            putStr(ioBfr);
        }
        else
        {
            putStr("ir");
        }
        putStr("\r\n");
        rs485TransmitDisa();
    }
}

 // ---------------------------
void respondUIDRequest(byte ch)
{
    extern char uniqueID[11];

    if (ch == pnetNodeNumber)
    {      
        rs485TransmitEna();
        putStr("uid");
        putChar('0' + pnetNodeNumber);
        putStr(uniqueID);
        putStr("\r\n");
        rs485TransmitDisa();
    }
}

   // --------------------------
void assignNodeNumber(byte childNodeNumberOld, byte childNodeNumber)
{
    extern byte pnetNodeNumber;
    if (childNodeNumberOld == pnetNodeNumber)
    {
        rs485TransmitEna();
        pnetNodeNumber = childNodeNumber;
        putStr("prak");
        putChar('0' + childNodeNumberOld);
        putChar('0' + childNodeNumber);
        putStr("\r\n");
        rs485TransmitDisa();
    }
}