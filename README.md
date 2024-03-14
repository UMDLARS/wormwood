# wormwood
It would be a really bad idea to write control software for a nuclear reactor in the C language. What if someone did?

## introduction
Wormwood is a nuclear reactor "simulator" exploring four major problems common to C programs:

 1. buffer overflow -- when more bytes are put into a buffer than the buffer has space for
 1. off-by-one errors -- when a counting or inequality issue results in a arithmetic mistake of 1
 1. integer overflow / underflow & sign -- when the "wrapping" of an integer type, or an incorrect sign causes something unexpected to happen
 1. string format vulnerabilities -- when a user is allowed to provide a format string to a `printf` statement.

The "simulation" (which is not even remotely accurate) has many issues, but you can cause something bad to happen using each of the four vulnerabilities described above.

## dependencies

Debian/Ubuntu: `sudo apt update && sudo apt install build-essential cmake libncurses-dev pkg-config`

Fedora/RHEL: `sudo dnf update && sudo dnf install gcc cmake ncurses-devel pkgconf`

ArchLinux: `sudo pacman -Syu && sudo pacman -S base-devel cmake ncurses`

## quickstart

This nuclear reactor simulator is a C program that can run on any Linux system. It does not require any privileges.

 1. Clone this repository onto a Linux machine. (For Spring'24 Computer Security, start a `posix` lab on Merge, log into `posix`, and do your work there.)
 1. Install dependencies above (Merge is using Debian).
 1. `cd` into the repository
 1. Execute `./run.sh`
 1. Log in as the `oper` user with the password `HomerSimpson`

Without looking at the source code, try to make the reactor fail using each of the four vulnerabilities.

## what does it look like?

When running, the program displays a "dashboard" for a nuclear reactor control system. It looks like this:

```
┌──────────────────────────────────────────────────────────────────────────────┐
│JERICHO NUCLEAR REACTOR STATUS PANEL                    (2024-03-12 16:30:35) │
├──────────────────────────────────────────────────────────────────────────────┤
│reactor temp:    70.05           coolant_temp:    64.57                       │
│rod_depth: 16 --[ [================] ]--  coolant flow rate: 10.00            │
│User: NA                                                                      │
├──────────────────────────────────────────────────────────────────────────────┤
│SAFETY PROTOCOLS:      [ENABLED] (Inactive)                                   │
└──────────────────────────────────────────────────────────────────────────────┘

Actions (choose one):
(A) - Authenticate
(Q) - Quit
Enter your selection (AQ) and then press ENTER.
```

## how do nuclear reactors work?

In short, nuclear reactors have enough radioactive material in a small space (the "containment vessel") that the radiation can cause a chain reaction, resulting in a release of more radiation -- including heat. The heat is absorbed by a cooling system, which generates steam to drive electricity-generating turbines. In addition to a cooling system (with some rate of coolant flow), reactors have "control rods" that can be inserted between the pieces of radioactive fuel. Control rods are cylinders of material that -- depending on how far the rods are inserted into the containment vessel -- absorb some of the radiation. The further the rods are inserted, the less radiation and the less heat. The less they are extended, the more radiation, the greater the chain reaction, and the more heat. If the rods are inserted all the way, the chain reaction stops.

Assuming the reactor vessel and coolant system remains sealed (and fuel and rods are sealed when removed from the vessel), radiation does not escape into the environment. However, reactors can fail in several catastrophic ways. One way they can fail is if the containment vessel is breached in any way -- that is, if a hole is somehow made in the vessel. If this happens, then radiation will escape. A vessel could be breached mechanically, e.g., if something stabbed the vessel hard enough, or if it has a "meltdown," which happens when a runaway chain reaction in the vessel generates so much heat that the mechanisms of the reactor or the vessel itself literally melt. (It's also challenging to clean up something that's so hot it melts everything in sight and is simultaneously incredibly radioactive.) Reactors can also fail if the pressure in the cooling system builds up to such a level (due to increase of heat) that the cooling system vents radioactive coolant into the air. (And of course, an active reactor without a cooling system will quickly melt down unless it is stopped via the control rods.) Radiation leaks cost many lives and can take 10s or 100s of years to clean up. Some famous ones include [Three Mile Island](https://en.wikipedia.org/wiki/Three_Mile_Island_accident), [Fukushima Daiichi](https://en.wikipedia.org/wiki/Fukushima_nuclear_accident), and of course, [Chernobyl](https://en.wikipedia.org/wiki/Chernobyl_disaster).

## how does this reactor control program work?

When started, the reactor operator is required to log in as an operator (`oper`), or as a supervisor (`super`). The password for the `oper` user is `HomerSimpson`. (The password for `super` is present in the sourcecode to `wormwood.c`, but of course an unprivileged operator would not know the password.) The operator can change the depth of the control rods or the flow rate of the coolant. This changes the temperature of the coolant and reactor. This (completely pretend) reactor is designed to function ideally between 1000 and 2000 degrees.

The reactor has an automatic safety feature: if the temperature of the reactor rises above 2,000 degrees, it will automatically insert the control rods and increase the coolant flow rate until the temperature dips below 2,000. 

Only the `super` user can disable the automated safety protocols, but normal `oper` users would not know the password (or have access to the control sourcecode).

This advanced and completely pretend reactor can continue operating until its temperature reaches 5,000 degrees, at which point it will fail catastrophically. (In reality, reactors [cannot withstand temperatures much above 1,000 degrees Fahrenheit](https://en.wikipedia.org/wiki/Corium_(nuclear_reactor)).)

The reactor can also operate in two different modes. In the first mode, called realtime, the reactor's temperature and other bits automatically update in the background every second. In the second mode, called norealtime, the reactor only updates when you perform an action, and an additional "wait" action is added. The default mode is realtime and can be overridden by passing "realtime" or "norealtime" as the first argument to `run.sh`.

## the vulnerabilities

There are many bugs in this program, but in particular there are four classic C-language issues in this program that can cause catastrophic failure of the "reactor".

### format string vulnerability

A [format string vulnerability](https://axcheron.github.io/exploit-101-format-strings/) happens when user input (which, as you know, should be considered metaphorically radioactive) is used as the argument to a `printf` statement. Users are able to provide a `printf` format string (e.g., `"%s"`) that, when executed by `printf` will find parameters on the stack and print them out. (This is because `printf` can accept an arbitrary number of parameters. You can also specify the nth parameter you want by providing the format string `%N$s` where `N` is an integer. Some values of `N` might crash your program, but other values of `N` will reveal information present on the stack of the running program. Where is it that you are able to provide a format string as input? You'll just have to figure that out by experimenting.

### off-by-one

Somewhere in the code is a vulnerability that has an arithmetic error where something should stop at a certain value, but it stops *one value away*. Unfortunately for the people who live near the reactor, that one unit turns out to be really significant.

### integer over/underflow and/or sign

Programmers often forget that in strongly typed languages, values will "roll over" if they are too high or too low. For example an `unsigned char` can represent values between 0-255. If you add one to an `unsigned char` 255, you'll get 0 -- not 256. Similarly, a `signed char` can represent values between -128 and 127. If you do 1 + 127 in signed `char`s, you'll get -128 -- not 128. Those numbers are *sign*ificantly different! Somewhat more simply, sometimes programmers use a *signed* variable in a context where *negative numbers do not make sense*; this can result in negative values being assigned in places where this makes no sense. (Always use an `unsigned` type if you don't need to support negative values, and obviously use the default signed types if you do.) After you find this bug in the reactor, they'll be putting up signs like ["This place is not a place of honor ... no highly esteemed deed is commemorated here... nothing valued is here" and "We considered ourselves to be a powerful culture"](https://en.wikipedia.org/wiki/Long-term_nuclear_waste_warning_messages).

### buffer overflow

A buffer overflow is simply when too much data is written into too small of a space. Traditionally, C would let you do this without a warning. These days, C compilers now will provide some warnings if they can tell at compile time that you're writing into a buffer that's too small for the data in question. Regardless, when you write beyond the boundaries of a buffer, you overwrite the memory after the buffer. As an analogy, if you feel that your apartment is too small and you knock out an interior wall, you're really only going to expand your apartment into the hallway or your neighbor's space, destroying whatever was there. This can corrupt values, such as stack values, that are next to the buffers in memory. If you overflow *enough* memory, you can overwrite the return address pointer that tells the function where to set the instruction pointer upon returning from the function. Setting the instruction pointer to non-code data, non-executable memory, or invalid memory will result in your program exiting. (Setting the return address to some malicious code that you overflowed onto the stack is a subject for a different lab.)

I think we can all agree that crashing the control system of a nuclear reactor could be bad for safety.

## tasks

### helpful information

All (intentional) vulnerabilities are contained within `wormwood.c` and `reactor.c`, however it may be helpful to look at some of the other headers (i.e., `console_win.h`) for information on some of the functions being used.

For faster testing you may want to use norealtime mode by running `./run.sh norealtime`. norealtime will let you update the reactor manually rather than having to wait a second for each update.

### exploitation

Make the nuclear reactor quit with a failure using each of the four vulnerabilities described above. For reference:

 1. Make the reactor program crash due to a buffer overflow corrupting the stack.
 2. Make the reactor fail due to an integer overflow or sign issue.
 3. Read the `super` user password out of program memory using a format string vulnerability. (The `super` user has a special privilege on the reactor that is especially dangerous.)
 4. Make the reactor fail due to an off-by-one error.

### remediation

Fix the source code so that it still functions normally, but:
 1. Buffer overflows are no longer possible (i.e., make sure that writes to a buffer cannot use more space than the buffer has)
 2. Add validation code and/or change the types of the variables so that integer overflow / sign issues do not lead to failures
 3. Format string vulnerabilities are no longer possible (i.e., don't use user data directly in `printf` statements)
 4. Fix the off-by-one error that results in the reactor failure.

### answers

Write your answers into the associated Canvas quiz item (available in the Canvas site for your class).

## questions / bug reports

Talk to your TA or email [pahp@d.umn.edu](mailto:pahp@d.umn.edu).
