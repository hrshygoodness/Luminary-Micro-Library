<!--
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
function FindAndSelect(selectBox, value)
{
  var i = 0;
  for(i = 0; i < selectBox.length; i++)
  {
    if(selectBox.options[i].value == value)
    {
      selectBox.selectedIndex = i;
      return;
    }
  }
}
function FindNearestAndSelect(selectBox, value)
{
  var i = 0;
  var min;
  var max;

  for(i = 0; i < selectBox.length; i++)
  {
    min = (value * 90) / 100;
    max = (value * 110) / 100;

    if((selectBox.options[i].value > min) && (selectBox.options[i].value < max))
    {
      selectBox.selectedIndex = i;
      return;
    }
  }
}
function SetFormDefaults()
{
  FindNearestAndSelect(document.config.br, br);
  FindAndSelect(document.config.stop, sb);
  FindAndSelect(document.config.bc, bc);
  FindAndSelect(document.config.flow, fc);
  FindAndSelect(document.config.parity, par);
  FindAndSelect(document.config.tnmode, tnm);
  FindAndSelect(document.config.tnprot, tnp);
  document.config.telnett.value = tt;
  document.config.telnetlp.value = tlp;
  document.config.telnetrp.value = trp;
  document.config.telnetip1.value =tip1;
  document.config.telnetip2.value =tip2;
  document.config.telnetip3.value =tip3;
  document.config.telnetip4.value =tip4;
  SetIPState();
}

function SetIPState()
{
  var tnmode = document.config.tnmode.value;
  var disable;
  if(tnmode == 0)
  {
    disable = true;
  }
  else
  {
    disable = false;
  }
  document.config.telnetip1.disabled = disable;
  document.config.telnetip2.disabled = disable;
  document.config.telnetip3.disabled = disable;
  document.config.telnetip4.disabled = disable;
  document.config.telnetrp.disabled = disable;
}
//-->
