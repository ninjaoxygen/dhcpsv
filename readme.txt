
                      dhcpsv / dhcpsvgui
		      
  What is it?
  -----------
  
  dhcpsv / dhcpsvgui is a minimal dhcp server for win32

  ** IMPORTANT **
  CAUTION - THIS SOFTWARE IS NOT DESIGNED FOR USE IN A HOSTILE ENVIRONMENT
  OR FOR PROVIDING A LONG TERM DHCP SERVER!

  This software is still very much ALPHA status, however has functioned
  correctly with the devices I have needed to use it with.

  dhcpsv is designed for situations where a small number of devices need
  to be given addresses via DHCP, e.g. when configuring a set of new hardware.

  It is given a "start addresss" and will allocate subsequent addresses to
  each unique device that requests an address.

  Latest Version
  --------------
  
  dhcpsv is hosted at https://github.com/ninjaoxygen/dhcpsv

  System Requirements
  -------------------
  
  dhcpsv is designed to run on Windows XP SP1 or later.
  
  The source code can be built with Microsoft Visual C++ Express 2008.
  
  Usage
  -----
  
  Run dhcpsvgui.exe then either use the right-click menu of the notification
  icon or press Win+Y to start the server, you will be prompted for the
  address to be allocated for the first client.
  
  The adapter, subnet and gateway will be determined automatically by looking
  at the active adapters on the local machine.

  The included dhcpsvgui project provides a gui for this, running as a
  notification area icon, with balloon notifications for server start, server 
  stop and each time a new lease is allocated.

  The addresses allocated are NOT persistent and are only remembered until
  exit. This means that if the dhcp server is re-started, in-use addresses may
  be allocated to a different device creating a conflict.

  Bug Reporting
  -------------
  
  Please e-mail me, Chris Poole <chris@hackernet.co.uk> to report bugs.
  As a bare minimum, please include a description of the the problem, what you
  expected to happen and the version of the operating system you are running.
  
  Copyright and License
  ---------------------

  This software is built around the following third party source:
  
  systemtraysdk.cpp / .h by Chris Maunder from CodeProject and is provided
  under the Code Project Open License (CPOL)
  For license see http://www.codeproject.com/info/cpol10.aspx 
  For original code see http://www.codeproject.com/KB/shell/systemtray.aspx
  
  rc resource files built using resedit
  http://www.resedit.net/
  
  All other files are released under the New BSD license.
  
  Copyright (c) 2010 - 2011, Chris Poole - chris@hackernet.co.uk
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of any associated organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


