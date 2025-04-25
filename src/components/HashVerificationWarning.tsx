import { Dialog, DialogFooter } from './ui/dialog';
import { Button } from './ui/button';
import { AlertTriangle, Shield, XCircle } from 'lucide-react';

interface HashVerificationWarningProps {
  open: boolean;
  onClose: () => void;
  modName: string;
  onContinue: () => void;
  onAbort: () => void;
}

export function HashVerificationWarning({
  open,
  onClose,
  modName,
  onContinue,
  onAbort
}: HashVerificationWarningProps) {
  return (
    <Dialog
      open={open}
      onClose={onClose}
      title="Security Warning: Hash Verification Failed"
      variant="error"
      maxWidth="md:max-w-2xl"
      showCloseButton={false}
    >
      <div className="space-y-4">
        <div className="flex gap-3 items-start">
          <div className="bg-destructive/20 p-2 rounded-full flex-shrink-0">
            <XCircle className="h-6 w-6 text-destructive" />
          </div>
          <div>
            <h3 className="font-semibold text-lg text-destructive">Hash Verification Failed for "{modName}"</h3>
            <p className="text-muted-foreground mt-1">
              The downloaded mod file does not match its expected cryptographic hash.
            </p>
          </div>
        </div>
        
        <div className="bg-muted/50 rounded-lg p-4 border border-border">
          <h4 className="font-medium mb-2 flex items-center">
            <AlertTriangle className="h-5 w-5 text-orange-500 mr-2" />
            Security Risks
          </h4>
          
          <ul className="list-disc pl-5 space-y-2 text-sm">
            <li>
              <span className="font-medium">Tampered Files:</span> The mod may have been altered from its original state, potentially introducing malicious code.
            </li>
            <li>
              <span className="font-medium">System Compromise:</span> Malicious mods can damage your game files or gain access to your system.
            </li>
            <li>
              <span className="font-medium">Data Loss:</span> Compromised mods could potentially delete or corrupt your saved games and other data.
            </li>
            <li>
              <span className="font-medium">Privacy Risks:</span> Modified mods might contain code that collects and transmits your personal information.
            </li>
            <li>
              <span className="font-medium">Performance Issues:</span> Even if not malicious, a mod with a failed hash verification might not function correctly or could cause crashes.
            </li>
          </ul>
        </div>

        <div className="bg-muted/50 rounded-lg p-4 border border-border">
          <h4 className="font-medium mb-2 flex items-center">
            <Shield className="h-5 w-5 text-primary mr-2" />
            Recommendations
          </h4>
          
          <ul className="list-disc pl-5 space-y-2 text-sm">
            <li>Install mods only from trusted sources and verified creators.</li>
            <li>Check if the mod repository has been updated recently and try downloading again.</li>
            <li>Contact the mod creator or repository maintainer to report this issue.</li>
            <li>Consider using alternative mods that pass hash verification.</li>
          </ul>
        </div>

        <p className="text-sm text-muted-foreground">
          Hash verification is a security measure that helps ensure the mod you're installing is exactly the same one the developer intended to share. When verification fails, it means the file has changed since it was published.
        </p>
      </div>

      <DialogFooter>
        <Button variant="outline" onClick={onAbort}>
          Abort Installation
        </Button>
        <Button variant="destructive" onClick={onContinue}>
          Install Anyway (Not Recommended)
        </Button>
      </DialogFooter>
    </Dialog>
  );
}