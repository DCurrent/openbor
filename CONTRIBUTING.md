
# Developer Guidelines

The following are universal rules for development. Just as members guidelines and rules, so do staff. If anything it is MORE important for staff as our actions have greater consequences across the forum. Please note the following rules for submitting code. Failure to adhere may result in loss of submission privileges.

 1. Documentation: This is absolutely paramount. Code must include robust commenting. Submissions to are to include a summery of the changes and their purpose. New features (and how to use them) are to be documented in the forum and preferably in Wiki. All documentations must be English.
 1. Formatting: Code must be properly formatted (preferably Allman Style).
 1. Structure Vs. Array: Never use static keyed arrays - always use structs instead.
 1. Magic Numbers: Never use literal constants (magic numbers). Always, ALWAYS use named constants for static values.
 1. Enumerators: Related to above, if you will be using a set of related constants, always create an enumerator.
 1. Professionalism: Please keep the source code and other documentations clean and professional. Also, check your ego at the door. We're here to collaborate and build upon each other. It's natural some will know more than others - this does not make you better or smarter. Nerd fights and edit wars will be not be tolerated.
 1. Author's Rule: Never add limitations or value caps to any variable numeric attribute. Someone will always think of a use that you didn't. If you must include a sanity check, do so at the point of execution, not read in. An example would be Landframe. Some authors use a negative value here for purposes of their own. Negative numbers would normally crash the landing logic, but we do not modify the author's requested value. Instead, there is a verification in the landing logic that ignores out of bounds values. This prevents issues but still allows the author's full creativity.
 1. Author's Responsibility: Just as author creativity is the law, so is author responsibility. Do not add pet rocks just to satisfy a minor request that is easily covered by script or to fix a problem from using features in unintended ways. Those things are the responsibility of the authors. Catering to every individual request would quickly result in an unstable mess of an engine.
 1. Compatibility: Never break backward compatibility without thorough discussion.

