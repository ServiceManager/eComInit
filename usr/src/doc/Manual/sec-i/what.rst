What is System XVI?
===================

System XVI, abbreviated *S16*, is a modern *system resource manager* for Linux
and the BSD distros. It manages system resources, ensuring that they are kept
running, self-repairing after faults, and providing the administrator with the
information necessary to make appropriate repairs if an un-self-reparable error
should occur.

It is designed in line with the Four Motives, which are detailed below:

The Four Motives
~~~~~~~~~~~~~~~~

- **Interface Orientation**: the system should be designed to fit a clean and
  stable interface. A well-designed interface makes for an obvious
  implementation.
- **Separation of Concerns**: individual components should not do much alone,
  but work in concert to create a grand system.
- **Modularity**: components should be easily replaceable and extensible.
- **Self-healing**: components that crash should be able to restart without
  forgetting system state or otherwise causing breakage.

Flexibility
~~~~~~~~~~~

System XVI is *generic*. It has been designed to allow for the management of
many different kinds of system resources in a uniform way. For example, it can
manage *background services* like *MongoDB* or *Node.js*,
*chronological services* like a regular temporary-file cleaner,
*internet services* like *vsftpd* or *telnetd*, as well as other kinds of
services.

Because it follows the Four Motives, all this is achieved in separate daemons;
each implements one section of the grand concert of System XVI. The flexibility
allows, for example, legacy unit-files from SystemD to be converted and ran by
System XVI.

Fault-Tolerant
~~~~~~~~~~~~~~

System XVI is fault-tolerant. If a resource should fail, System XVI will try to
make that resource available again, restarting services as needed to restore
its functionality. In the case a severe fault occurs which cannot be repaired
automatically, a clear description of the fault will be assembled and made
available to the administrator for their viewing.
