#!/usr/bin/python

from termcolor import colored
import sys, os

LOG_FILE = sys.stdout
stdout = sys.stdout
stderr = sys.stderr

def Init(logname):
    os.remove(logname) if (os.path.exists(logname)) else None
    global LOG_FILE
    LOG_FILE = open(logname, "a")
    sys.stdout = LOG_FILE
    sys.stderr = LOG_FILE

def Next():
    stdout.write("\n")
    LOG_FILE.write("\n")
    LOG_FILE.flush()

def Debug(*args):
    LOG_FILE.write("debug: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.flush()

def Info(*args):
    stdout.write("info: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.write("info: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.flush()

def Warning(*args):
    stderr.write("warning: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.write("warning: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.flush()

def Error(*args):
    stderr.write("error: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.write("error: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.flush()

def Fatal(*args):
    stderr.write("fatal: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.write("fatal: " + ' '.join([str(a) for a in args]) + "\n")
    LOG_FILE.flush()
