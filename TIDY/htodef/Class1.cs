using System;
using System.IO;
using System.Text.RegularExpressions;

namespace htodef {
	class Class1 {
		[STAThread]
		static void Main(string[] args) {
			Console.WriteLine("LIBRARY tidy");
			Console.WriteLine("EXPORTS");

			try {
				// Caveat: comments will not correctly parse  /* /* recursive */ c-style */ comments
                Regex comments = new Regex(@"(/\*.*?\*/)|(//.*?$)", RegexOptions.Compiled | RegexOptions.Singleline | RegexOptions.Multiline);
                Regex exports = new Regex(@"^TIDY_EXPORT\s+\S+\s+TIDY_CALL\s+([^\s\(]+)", RegexOptions.Compiled | RegexOptions.Multiline);

				foreach(string s in args) {
					string contents = File.ReadAllText(s);

                    contents = comments.Replace(contents, "");

                    MatchCollection matches = exports.Matches(contents);
                    foreach (Match m in matches) {
                        Console.WriteLine("   " + m.Groups[1].Value);
                    }
				}
			}
			catch(Exception ex) {
				Console.Error.WriteLine("Exception Occured: "+ex.ToString());
			}
		}
	}
}
