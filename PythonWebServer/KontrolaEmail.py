from re import sub
import smtplib
import imaplib
import requests
import time

def sendEmail():
    while True:
        CHANNEL_ID = 1727854
        TS_KEY = "IIQHKU7U0V65VD2A"

        resultFromTs = requests.get("https://api.thingspeak.com/channels/{}/feeds.json?api_key={}".format(CHANNEL_ID, TS_KEY))

        data = resultFromTs.json()['feeds']

        sumTemp = 0
        sumBri = 0
        totalDoor = 0
        totalRelay = 0

        for one in data:
            sumTemp += float(one['field1'])
            averageTemp = sumTemp / len(data)

            sumBri += float(one['field2'])
            averageBri = sumBri / len(data)

            totalDoor += float(one['field3'])

            totalRelay += float(one['field4'])

        server = smtplib.SMTP('smtp.gmail.com', 587)
        server.starttls()

        EMAIL_ADDRESS = input("Unesite email adresu posiljioca: ")
        PASSWORD = input("Unesite sifru posiljioca: ")

        DESTINATION_EMAIL_ADDRESS = input("Unesite email adresu primaoca: ")

        resCode = server.login(EMAIL_ADDRESS, PASSWORD)

        subject = "Dnevni Izvestaj o Senzorima - Bozidar Mladenovic"

        body =  "Prosecna Vrednost Temperature: {} C, \n".format(round(averageTemp, 2))
        body += "Prosecna Vrednost Osvetljenja: {} %, \n".format(round(averageBri, 2))
        body += "Ukupan Broj Otvaranja Vrata: {}, \n".format(totalDoor)
        body += "Ukupan Broj Promena Stanja Na Releju: {} \n".format(totalRelay)

        fullEmail = "Subject: {}\n\n{}".format(subject, body)

        resCode = server.sendmail(from_addr=EMAIL_ADDRESS, to_addrs=DESTINATION_EMAIL_ADDRESS, msg=fullEmail)

        server.quit()

        time.sleep(86400)

#sendEmail()