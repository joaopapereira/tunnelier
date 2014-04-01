import json
from optparse import OptionParser
import urllib2
import sys
usage = "usage: %prog [options] action to_send expected"
parser = OptionParser(usage = usage)
parser.add_option("-a", "--address", dest="server_address",
                  help="Server Address IP", default="localhost")
parser.add_option("-p", "--port", dest="server_port",
                  help="Server Port IP", default="10000")

parser.add_option("-c", "--http_code", dest="http_code",
                  help="Expected HTTP code", default="200")
(options, args) = parser.parse_args()
print options
print args

server_address = options.server_address
server_port = options.server_port
action = args[0]
to_send = args[1]
expected = json.loads(args[2])
url = 'http://%s:%s/%s'%(server_address,server_port,action)
print url
req = urllib2.Request(url)

req.add_header('Content-Type', 'application/json')
try:
   response = urllib2.urlopen(req, to_send)
   j_response = json.loads(response.read())
except urllib2.HTTPError as e:
    assert e.code == int(options.http_code), "HTTP Code error: %s when expected %s"%(e.code, options.http_code)
    j_response = json.loads(e.read())

assert expected == j_response, "Response '%s' expected, obtained: '%s'"%(expected,j_response)

sys.exit(0)
