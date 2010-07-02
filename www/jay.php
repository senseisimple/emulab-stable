<?
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("In Memoriam");

echo "<span class='picture'><img src=jay.jpg><br></span><br>\n";

?>
<blockquote><p>
<center><b><font color=red size=+1>Jay Lepreau<br>03/27/1952 -- 09/15/2008
</font></b></center>
<p>
We are
<a href='http://www.legacy.com/saltlaketribune/Obituaries.asp?Page=Lifestory&PersonId=117597321'>sad to report</a>
that Jay Lepreau, Research Professor and Director 
of the Flux Research Group, passed away Monday morning Sept 15th due to
complications of cancer. Jay was an enthusiastic and productive
researcher, a dedicated mentor of students and staff, and an avid
participant in recreational activities such as music and outdoor
sports. His loss will be felt by all who knew him, both  within the
computer science community and elsewhere. 
</p>
<p>
What follows is a reprint of a tribute article written by David
Andersen, to appear in an upcoming issue of "ACM Computer
Communication Review."
</p>

<br>
<blockquote><blockquote>
<em>
"I don't want to break precedent or anything by starting this on time,
or doing anything other than at the last minute..."<br>
  -- Jay Lepreau, OSDI Welcome Comments, 1994.
</em>
</blockquote></blockquote>

<p>
Jay Lepreau was uncharacteristically early in his passing, but left
behind him a trail of great research, rigorously and repeatably
evaluated systems, and lives changed for the better.
</p>

<p>
Most in our community know Jay as the creator of Emulab; the more
systems-oriented may also know that he conceived and served as the
first program chair for OSDI in 1994.  Those in the security community
see the path that his work in microkernels took through Fluke, to
Flask, to today's SELinux.  While diverse and prolific, Jay's research
can also be easily summed up with a few traits: he devoted his
boundless energy to creating real systems that had major practical
impact.  In fact, Jay and his research group, heavy in talented
systems programmers, were unique among the academic community in their
willingness and ability to create and maintain artifacts that were
used, over decades, by thousands of other researchers.  Emulab alone
revolutionized the evaluation sections of networking and distributed
systems papers, simultaneously setting a high bar for rigorous
evaluation while also making that bar achievable.
</p>

<p>
Jay was born March 27, 1952, to Dr. Frank J. Lepreau, Jr., and Miriam
Barwood.  He spent his childhood both in Massachusetts, and in Central
Haiti, receiving a lasting
education in charity, kindness, and medical care helping his father
treat patients at the Hospital Albert Schweitzer.
His care for others eventually brought him to Utah, where he began
working with the Southern Utah Wilderness Alliance to protect the
red-rock deserts of southern Utah, and at twenty eight, he entered the
Department of Computer Science at the University of Utah as an
undergraduate.
</p>

<p>
He got his start in systems programming and real systems while still
an undergraduate, hanging around the computing facility and then
officially working as a programmer for the then-director of computing
facilities, Randy Frank.  Jay graduated in 1983 and became the manager
of the systems programming group.  Randy and Jay established the first
Ethernet networks on campus, and together ran the USENIX 1984 Summer
Conference.  Within four years, Jay was the acting head of computing
facilities for the CS department, learning the impossible task of
trying to satisfy a diverse and intelligent group of strong-willed,
impatient users who know, or believe they know, as much as the
facilities director---while trying to keep the computers running 24x7
on a limited budget.  These were, in other words,
the exact same challenges that
Jay would surmount so well in making Emulab the resource we all depend
upon today.
</p>

<p>
Jay's research accomplishments are many, though in the age of Google
it seems unnecessary to belabor them, aside from noticing a
suspiciously alliterative pattern that was the hallmark of his Flux
group for many years---the Flux OSKit, which dissected the available
UNIX-like OSes of its day into components that could be reassembled at
will; the Fluke microkernel; the Flask secure microkernel; the Flick
optimizing IDL compiler; and the Flexlab replay and emulation
additions to Emulab. Jay got started earlier than these projects,
receiving funding from HP to port the BSD operating system to PA-RISC.
For years following, Jay proudly noted that his group at Utah was the
first and last site to run HP-BSD.  Jay's team then did major
development on the Mach microkernel.  Here too did Jay's emphasis on
real systems and impact show up: the Flux group had successfully
ported the full GNU toolchain to run on their Mach-based microkernel,
and again on the Fluke microkernel.
</p>

<p>
But for many of us, Jay's research accomplishments were merely a
wonderful byproduct of his people accomplishments.  In talking to
people about Jay, a common theme that emerges is that Jay was a
linchpin in hooking us in to a career in computer science.  Bryan
Ford, now joining the faculty at Yale, wrote:
</p>

<blockquote><blockquote>
<em>
"Before meeting him, I was just a young geek with ideas and a fancy
for operating systems; he invited me into his group, gave me a
workstation named after a brand of liquor, infected me with his
passion for systems research, taught me how to develop and express the
crazy ideas we came up with together, and patiently nurtured me into a
member of the global systems community.  No one else in the world
deserves my gratitude more for getting me started on my professional
path."
</em>
</blockquote></blockquote>

<p>
Jay leaves a trail of people he has touched in this way; at one point,
two of the systems graduate students at MIT had worked with Jay as
undergraduates, and another had worked with Jay as a high school
student---the latter of whom became, to my knowledge, the first high
school student to give a work-in-progress talk at SOSP. Jay's former
students, staff, and visitors now represent faculty at four
universities, current grad students at nine top CS departments, and
occupy spots at nearly all of the largest computing companies in the
United States.  This legacy is unsurprising---a large part of Jay's
brilliance was his ability to forge a team that coupled freely-flowing
idea sources to extremely talented and experienced implementors.
</p>

<blockquote><blockquote>
<em>
"He leaves behind a great team of people and a legacy of tools and
principles in the area of experimental networking research that will
live far longer than most other tools developed in academia. Jay
worked tirelessly towards making Emulab and Flux hugely successful. To
say the least, we'll miss him dearly."<br>
-- Shashi Guruprasad, now at Cisco.
</em>
</blockquote></blockquote>

<p>
When Jay was diagnosed with cancer, he decided that he wanted to send
his last major project, Emulab, off with a bang---and directed his
entire, formidable energy to it despite the ravages of chemotherapy.
Jay put off telling most people about his cancer for quite some time
to make sure that ``his problems'' did not get in the way of Emulab's
flourishing---and, I suspect, out of a deep-seated sense of humility
and desire not to burden others.  Rick McGeer noted:
</p>

<blockquote><blockquote>
<em>
"I vividly remember one night in February when we were putting
finishing touches on a proposal together. We were doing what research
computer scientists do---revising text, checking budgets, emailing
each other final last-minute edits as the proposal deadline got
closer and closer. I remember looking up at the clock in my study in
California and 2 AM, and thinking to myself, ``I'm 51 years old, too
old to be doing this nonsense---I'm stupid. But it's 3 AM where Jay
is, and he has *cancer*. Now that man is crazy''. I didn't say it to
Jay, unfortunately---he would have gotten a kick out of that."
</em>
</blockquote></blockquote>

<p>
Jay's favorite maxim was from Voltaire:
<blockquote><blockquote>
<em>
"The perfect is the enemy of the good."
</em>
</blockquote></blockquote>

and he was never shy to share this advice with a student or programmer
who couldn't step back and see that the issue they were sweating had
little to do with the big picture of the project.  Jay's other advice,
implicit and explicit, was just as practical: It is easier to ask
forgiveness than permission.  Any paper worth submitting should be
started at least the night before the deadline, and sometimes earlier.
An eggshell ground pad makes an excellent emergency bed for the
office.  Despite working insane hours, make time for a two-week river
rafting trip in southern Utah to be with your family in a place you
love.  Be passionate about what you do, and do what you're passionate
about.
</p>

<p>
In the first OSDI (1994), Jay and the program committee invited David
Patterson as the keynote speaker.  Patterson gave his now-famous talk
on ``How to Have a Bad Career in Academia,'' where he noted seriously
that the alternative is to ``change the way people do computer science
and engineering.''  Clearly, Jay took this advice to heart.  In his
program chair's introduction, Jay wrote: ``My baby is born; from now on
it's up to others to raise it well.''  The same could be said for
Emulab, for the many students and staff who Jay helped send into the
world, and for the red-rock deserts of Southern Utah.  Jay's work is
done; now it's up to us.
</p>
</blockquote>

<?php
PAGEFOOTER();
?>
