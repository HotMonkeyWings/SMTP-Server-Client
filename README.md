# SMPTP Server
SMTP server and client application implemented in C.
![Screenshot](/Screenshot.png)
## Prerequisites
To run the application, you will need to compile the source code. The packages that are required are
- `gcc` - The GNU compiler collection

## Usage

To run the applications, you first need to clone this repository and change the current working directory to the cloned repo.

```
$ git clone https://github.com/HotMonkeyWings/SMTP-Server-Client.git
$ cd SMTP-Server-Client/
```

Then, compile the program using `gcc`
```
$ gcc server.c -o Server -lpthread
$ gcc client.c -o Client
```

Enter the following command to run the server, replacing '3000' with any desired Port number outside the range of 0-1024. If not given, '8080' will be taken as the default Port Number.

```
$ ./Server 4200
```

Next, run the client and connect to the same Port number. 
```
$ ./Client 4200
```

A login screen would appear on the client application. On entering authenticating with a correct credentials, the application shows a menu with the options available.


## Input Format

When the Send Mail option is selected, the client application asks for the **From email**, **To email** and **the Subject line**.

Right after the subject line, you can start typing the Email's body.
To end reading the body, type a line with just '.' and press Enter, sending the mail to the recepient.

An example of an email input would be
```
From: Andrew@localhost
To: Stallings@localhost
Subject: SMTP Experiment
Next evaluation will be on 22 March 2021.
Please be ready..
. 
```

## Mailbox Storage

All the emails are stored inside the current working directory of the server application.

For example, the mailbox of a user - `username` would be stored inside the file `username/mymailbox.mail`.

The file contains all the emails separeted by period line (a line with just a `.`) with the following format.

```
From: sender@localhost
To: username@localhost
Subject: Email Subject
Received: Tue, 23 Mar 2021 06:12:15 GMT
Email Body
.

From: sender@localhost
To: username@localhost
Subject: Email Subject
Received: Fri, 26 Mar 2021 06:77:15 GMT
Email Body
.
```
