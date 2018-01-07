const char HTTP_DEFAULTHTML[] PROGMEM = R"=====(
<div style='text-align:left;display:inline-block;min-width:260px;'>
  <div class='fbg'>
    <div class='l lt'>
      <label>{v}</label>
      <hr />
    </div>
    <table>
      <tr>
        <td>
          <button name='btnR1' onclick='SetState("/set?ch=1&state=1&ts=1"); return false;'>AN</button>
        </td>
        <td>
          <button name='btnR2' onclick='SetState("/set?ch=2&state=1&ts=1"); return false;'>AN</button>
        </td>
      </tr>
      <tr class='ls'>
        <td>
          <label id='_ls1'>{ls1}</label>
        </td>
        <td>
          <label id='_ls2'>{ls2}</label>
        </td>
      </tr>
      <tr>
        <td>
          <button name='btnR1' onclick='SetState("/set?ch=1&state=0&ts=1"); return false;'>AUS</button>
        </td>
        <td>
          <button name='btnR2' onclick='SetState("/set?ch=2&state=0&ts=1"); return false;'>AUS</button>
        </td>
      </tr>
    </table>

    <div></div>
    <hr />
    <div></div>
    <div>
      <input class='lnkbtn' type='button' value='Konfiguration' onclick="window.location.href='/config'" />
    </div>
    <div class='l c k'>
      <label>Firmware: {fw}</label>
    </div>
    <div>
      <input class='fwbtn' id='fwbtn' type='button' value='Neue Firmware verf&uuml;gbar' onclick="window.open('{fwurl}')" />
    </div>
    <div>
      <input class='fwbtn' id='fwbtnupdt' type='button' value='Firmwaredatei einspielen' onclick="window.location.href='/update'" />
    </div>
  </div>
)=====";

