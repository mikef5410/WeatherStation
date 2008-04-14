#
# $Id$

INST_DIR=/home/httpd/html/wxd
USER=nobody
GROUP=nobody

install: split-query.cgi interval-query.cgi
	install --group=$(GROUP) --owner=$(USER) -m0555 split-query.cgi  interval-query.cgi $(INST_DIR)
