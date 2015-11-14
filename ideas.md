# Idea
## File open time limits
- extended file attributes
- timestamp set to midnight of everyday to reset time
- "opened at" timestamp
  - set when counter for number of current opens is less than or equal to 0
- counter for number of current opens
  - if it reaches 0 update the time open
  - reduce when closed is called
  - increase when open is called
- if total time open is greater than allowed time, refuse any future opens